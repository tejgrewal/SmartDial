
#include "Aim.h"
#include "AppConfig.h"
#include "Theme.h"
#include "Widgets.h"
#include "Haptics.h"
#include <TFT_eSPI.h>
#include <Modulino.h>
#include <Preferences.h>

// ---- Optional I2C touch (CHSC6X) ----
#ifdef USE_CHSC6X_TOUCH
  #include <Wire.h>
  #ifndef CHSC6X_I2C_ID
    #define CHSC6X_I2C_ID 0x2e
  #endif
  #ifndef TOUCH_INT
    #define TOUCH_INT D7
  #endif
  #ifndef CHSC6X_READ_POINT_LEN
    #define CHSC6X_READ_POINT_LEN 5
  #endif
#endif

using namespace Widgets;

extern TFT_eSPI tft; // needed for rotation and/or fallback getTouch

namespace Aim{

// ----------------- config -----------------
static const int TARGET_R = 26;
static const uint32_t COUNTDOWN_MS = 1600;
static const uint32_t TLIM_START   = 1100;
static const uint32_t TLIM_MIN     = 450;
static const uint32_t TLIM_DEC     = 20;

// ----------------- state ------------------
static ModulinoKnob* k = nullptr;   // for menu navigation
static int lastKnobPos = 0;

enum class GState { READY, COUNTDOWN, RUNNING, GAME_OVER, PAUSED };
static GState gs = GState::READY;

static int score = 0;
static int best  = 0;
static int lives = 3;
static int combo = 0;
static int roundN = 0;

static int txC = CX, tyC = CY;  // target center
static uint32_t tAppear = 0;
static uint32_t tLimit  = TLIM_START;
static uint32_t tStart  = 0; // countdown start

// pause/overlay menu
static bool showMenu = false;
static int  sel = 0;
enum class MenuType { PAUSE, GAMEOVER };
static MenuType menuType = MenuType::PAUSE;
static int menuCount = 0;

// ----------------- utils ------------------
static inline int irand(int a,int b){ return a + (int)(random() % (long)(b-a+1)); }

#ifdef USE_CHSC6X_TOUCH
static inline void touchInitOnce(){
  static bool inited=false;
  if(inited) return;
  pinMode(TOUCH_INT, INPUT_PULLUP);
  Wire.begin();
  inited=true;
}

// replicate reference get_xy with rotation mapping
static bool readTouchCHSC6X(int &x, int &y){
  touchInitOnce();
  // Only read if interrupt is low (pressed)
  if (digitalRead(TOUCH_INT) != LOW) return false;

  uint8_t temp[CHSC6X_READ_POINT_LEN] = {0};
  uint8_t read_len = Wire.requestFrom((uint8_t)CHSC6X_I2C_ID, (uint8_t)CHSC6X_READ_POINT_LEN);
  if (read_len != CHSC6X_READ_POINT_LEN) return false;

  Wire.readBytes(temp, CHSC6X_READ_POINT_LEN);
  if (temp[0] != 0x01) return false;

  int32_t raw_x = temp[2];
  int32_t raw_y = temp[4];

  // map with rotation like reference
  uint8_t rot = tft.getRotation();
  switch(rot){
    default:
    case 0: x = raw_x; y = raw_y; break;
    case 1: x = raw_y; y = SCREEN_WIDTH  - raw_x; break;
    case 2: x = SCREEN_WIDTH  - raw_x; y = SCREEN_HEIGHT - raw_y; break;
    case 3: x = SCREEN_HEIGHT - raw_y; y = raw_x; break;
  }
  return true;
}
#endif

static bool readTouch(int &x, int &y){
#ifdef USE_CHSC6X_TOUCH
  int tx,ty;
  if (readTouchCHSC6X(tx,ty)) { x=tx; y=ty; return true; }
#endif
  // fallback to TFT_eSPI touch (if configured) -- note int32_t* signature
  int32_t rx = 0, ry = 0;
  if (tft.getTouch(&rx, &ry)){ x = (int)rx; y = (int)ry; return true; }
  return false;
}

static bool insideTarget(int x, int y){
  int dx = x - txC;
  int dy = y - tyC;
  return (dx*dx + dy*dy) <= (TARGET_R*TARGET_R);
}

static void spawn(uint32_t now){
  const int pad = 28;
  txC = irand(pad, SCREEN_WIDTH - pad);
  tyC = irand(pad + 12, SCREEN_HEIGHT - pad);
  tAppear = now;
}

static int pointsFor(uint32_t rtMs){
  if(rtMs <= 180) return 100;
  if(rtMs <= 260) return 75;
  if(rtMs <= 360) return 50;
  if(rtMs <= 520) return 25;
  return 10;
}

// ----------------- local highscore (Preferences) ----------------
static void loadBest(){
  Preferences p;
  p.begin("qubeGames", true);
  best = p.getInt("aimHigh", 0);
  p.end();
}
static void saveBest(){
  if (score > best){
    best = score;
    Preferences p;
    p.begin("qubeGames", false);
    p.putInt("aimHigh", best);
    p.end();
  }
}

// ----------------- lifecycle --------------
void init(TFT_eSprite &spr, ModulinoKnob &knob){
  (void)spr;
  k = &knob;
  lastKnobPos = knob.get();
  loadBest();
  reset();
  gs = GState::COUNTDOWN;
  tStart = millis();
  showMenu = false;
  sel = 0;
  menuType = MenuType::PAUSE;
  menuCount = 0;
}

void reset(){
  score = 0;
  lives = 3;
  combo = 0;
  roundN = 0;
  tLimit = TLIM_START;
  gs = GState::COUNTDOWN;
  tStart = millis();
  showMenu = false;
  sel = 0;
  menuType = MenuType::PAUSE;
  menuCount = 0;
}

// knob controls only when menu open
static void updateMenuKnob(){
  if (!showMenu || !k) return;
  int pos = k->get();
  int d = pos - lastKnobPos;
  lastKnobPos = pos;
  if (d > 6) d = 6;
  else if (d < -6) d = -6;
  if (d != 0 && menuCount > 0){
    sel = (sel + (d > 0 ? 1 : -1) + menuCount) % menuCount;
    Haptics::tap();
  }
}

// ----------------- tick -------------------
void tick(float dt){
  (void)dt;

  // countdown -> running
  if (gs == GState::COUNTDOWN){
    if (millis() - tStart >= COUNTDOWN_MS){
      gs = GState::RUNNING;
      spawn(millis());
    }
    return;
  }

  if (gs != GState::RUNNING) {
    updateMenuKnob();
    return;
  }

  // running: handle timeout
  uint32_t now = millis();
  if (now - tAppear >= tLimit){
    lives--;
    combo = 0;
    Haptics::back();
    if (lives <= 0){
      gs = GState::GAME_OVER;
      saveBest();
    } else {
      spawn(now);
    }
  }

  // handle taps
  int x,y;
  if (readTouch(x,y)){
    // top-right "pause" box hit region
    if (x >= SCREEN_WIDTH-30 && x <= SCREEN_WIDTH-4 && y >= 4 && y <= 30){
      // open pause menu
      gs = GState::PAUSED;
      showMenu = true;
      sel = 0;
      menuType = MenuType::PAUSE;
      menuCount = 3;
      Haptics::tap();
      return;
    }

    if (insideTarget(x,y)){
      uint32_t rt = now - tAppear;
      int pts = pointsFor(rt);
      score += pts + (combo * 5);
      combo++;
      Haptics::tap();
      // tighten time limit
      if (tLimit > TLIM_MIN){
        tLimit -= TLIM_DEC;
        if (tLimit < TLIM_MIN) tLimit = TLIM_MIN;
      }
      roundN++;
      spawn(now);
    }
  }
}

// ----------------- draw -------------------
static void drawHUD(TFT_eSprite &spr){
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  spr.drawString("AIM", 6, 4);

  char b[32];
  spr.drawString((snprintf(b,sizeof(b),"Score %d",score), b), 70, 4);
  spr.drawString((snprintf(b,sizeof(b),"Best %d",best), b), 170, 4);

  // lives bottom-left
  int lx = 10, ly = SCREEN_HEIGHT - 12;
  for (int i=0;i<3;i++){
    uint16_t c = (i<lives) ? TFT_RED : TFT_DARKGREY;
    spr.fillCircle(lx + i*16, ly, 6, c);
    spr.drawCircle(lx + i*16, ly, 6, TFT_BLACK);
  }

  // pause/home box top-right
  const int hw=26, hh=26, hx=SCREEN_WIDTH-hw-4, hy=4;
  spr.drawRoundRect(hx, hy, hw, hh, 5, TFT_DARKGREY);
  int mx=hx+hw/2, my=hy+hh/2;
  spr.drawLine(mx-7,my+6, mx-7,my-3, TFT_DARKGREY);
  spr.drawLine(mx+7,my+6, mx+7,my-3, TFT_DARKGREY);
  spr.drawLine(mx-9,my-3, mx,  my-12, TFT_DARKGREY);
  spr.drawLine(mx+9,my-3, mx,  my-12, TFT_DARKGREY);
}

static void drawCountdown(TFT_eSprite &spr){
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  uint32_t el = millis() - tStart;
  const char* txt;
  if (el < 400) txt = "3";
  else if (el < 800) txt = "2";
  else if (el < 1200) txt = "1";
  else txt = "GO!";
  spr.drawString(txt, CX, CY);
}

static void drawTarget(TFT_eSprite &spr){
  // bullseye
  spr.fillCircle(txC, tyC, TARGET_R+10, 0xFBAE);
  spr.fillCircle(txC, tyC, TARGET_R+6,  TFT_WHITE);
  spr.fillCircle(txC, tyC, TARGET_R+2,  0xFD20);
  spr.fillCircle(txC, tyC, TARGET_R-4,  TFT_WHITE);
  spr.fillCircle(txC, tyC, TARGET_R-10, 0xFD20);

  // urgency bar bottom
  uint32_t now = millis();
  uint32_t elapsed = now - tAppear;
  uint32_t left = (elapsed >= tLimit) ? 0 : (tLimit - elapsed);
  int barW = SCREEN_WIDTH - 20;
  int leftW = (int)((barW * (float)left) / (float)tLimit);
  spr.drawRoundRect(10, SCREEN_HEIGHT-26, barW, 10, 3, TFT_DARKGREY);
  spr.fillRoundRect(10, SCREEN_HEIGHT-26, (leftW>0?leftW:0), 10, 3, (left<250)?TFT_RED:TFT_GREEN);
}

static void drawMenuBox(TFT_eSprite &spr, const char* title,
                        const char* const* items, int count, int selIdx){
  spr.fillRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Theme::BG);
  const int w=180, h=110;
  const int x=CX-w/2, y=CY-h/2;
  spr.fillRoundRect(x,y,w,h,8, Theme::BG);
  spr.drawRoundRect(x,y,w,h,8, TFT_DARKGREY);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_CYAN, Theme::BG);
  spr.drawString(title, CX, y+16);
  spr.setTextDatum(TL_DATUM);
  for(int i=0;i<count;i++){
    int iy = y + 36 + i*20;
    if (i==selIdx){
      spr.fillRoundRect(x+10, iy-2, w-20, 18, 4, TFT_DARKGREY);
      spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
    } else {
      spr.setTextColor(TFT_LIGHTGREY, Theme::BG);
    }
    spr.drawString(items[i], x+16, iy);
  }
}

static void drawPauseMenu(TFT_eSprite &spr){
  static const char* items[] = { "Resume", "Replay", "Main Menu" };
  drawMenuBox(spr, "PAUSED", items, 3, sel);
}

static void drawGameOverMenu(TFT_eSprite &spr){
  static const char* items[] = { "Replay", "Main Menu" };
  drawMenuBox(spr, "GAME OVER", items, 2, sel);
}

void draw(TFT_eSprite &spr){
  spr.fillSprite(Theme::BG);

  if (gs == GState::RUNNING){
    drawHUD(spr);
    drawTarget(spr);
  } else if (gs == GState::GAME_OVER){
    drawHUD(spr);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_ORANGE, Theme::BG);
    spr.drawString("Game Over", CX, CY-20);
    char b[48];
    snprintf(b, sizeof(b), "Score %d  Best %d", score, best);
    spr.drawString(b, CX, CY+12);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    spr.drawString("Press to return", CX, CY+36);
  } else if (gs == GState::COUNTDOWN){
    drawHUD(spr);
    drawCountdown(spr);
  } else if (gs == GState::PAUSED){
    drawHUD(spr);
    if (!showMenu){
      spr.setTextDatum(MC_DATUM);
      spr.setTextColor(TFT_CYAN, Theme::BG);
      spr.drawString("PAUSED", CX, CY);
    }
  } else {
    drawHUD(spr);
  }

  if (showMenu){
    if (menuType == MenuType::PAUSE) drawPauseMenu(spr);
    else                             drawGameOverMenu(spr);
  }

  spr.pushSprite(0,0);
}

// ----------------- button press -------------
void onPress(void (*goMenu)()){
  if (showMenu){
    if (menuType == MenuType::PAUSE){
      if (sel == 0){        // Resume
        showMenu = false;
        gs = GState::RUNNING;
        Haptics::tap();
      } else if (sel == 1){ // Replay
        reset();
        showMenu = false;
        Haptics::press();
      } else {              // Main Menu
        showMenu = false;
        Haptics::back();
        goMenu();
      }
    } else { // GAMEOVER menu
      if (sel == 0){
        reset();
        showMenu = false;
        Haptics::press();
      } else {
        showMenu = false;
        Haptics::back();
        goMenu();
      }
    }
    return;
  }

  if (gs == GState::COUNTDOWN){
    return; // ignore presses during countdown
  }

  if (gs == GState::RUNNING){
    gs = GState::PAUSED;
    sel = 0;
    showMenu = true;
    menuType = MenuType::PAUSE;
    menuCount = 3;
    Haptics::tap();
  } else if (gs == GState::GAME_OVER){
    showMenu = true;
    sel = 0;
    menuType = MenuType::GAMEOVER;
    menuCount = 2;
  }
}

} // namespace Aim

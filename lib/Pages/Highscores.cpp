#include "Highscores.h"
#include "AppConfig.h"
#include "Storage.h"
#include "Widgets.h"
#include "Haptics.h"
#include "Icons.h"
#include "Blit.h"
#include "Theme.h"
#include <Preferences.h>

static int lastKnob = 0;
static int sel = 0; // index into ITEMS
static const int ICON_SIZE = 40;
static uint32_t s_popStartMs = 0;

// Cache AIM high once (avoid hitting NVS every frame)
static int s_bestAim = 0;

enum ItemType { IT_BACK=0, IT_PONG, IT_SNAKE, IT_AIM, IT_RESET, IT_COUNT };

struct Item {
  const char* name;
  const uint16_t* icon;
  int w, h;
  uint16_t colorkey;
  ItemType type;
};

// NOTE: Pong uses infinityIcon in this project
static const Item ITEMS[IT_COUNT] = {
  { "Back",         backIcon,     backIcon_W,     backIcon_H,     backIcon_COLORKEY,     IT_BACK  },
  { "Pong",         infinityIcon, infinityIcon_W, infinityIcon_H, infinityIcon_COLORKEY, IT_PONG  },
  { "Snake",        snakeIcon,    snakeIcon_W,    snakeIcon_H,    snakeIcon_COLORKEY,    IT_SNAKE },
  { "AIM",          aimIcon,      aimIcon_W,      aimIcon_H,      aimIcon_COLORKEY,      IT_AIM   }, // <-- NEW
  { "Reset Scores", nullptr,      0,              0,              0,                     IT_RESET }
};

static inline void startPop(){ s_popStartMs = millis(); }
static inline float popT(){
  float t = (millis() - s_popStartMs) / 180.0f;
  if (t > 1.f) t = 1.f;
  return t;
}

// Load AIM high score from Preferences (same namespace/key used by AIM.cpp)
static void loadAimBest(){
  Preferences p;
  p.begin("qubeGames", true);
  s_bestAim = p.getInt("aimHigh", 0);
  p.end();
}

// Zero AIM high score in Preferences
static void resetAimBest(){
  Preferences p;
  p.begin("qubeGames", false);
  p.putInt("aimHigh", 0);
  p.end();
  s_bestAim = 0;
}

void Highscores::init(TFT_eSprite &, ModulinoKnob &knob){
  lastKnob = knob.get();
  sel = 0;
  loadAimBest();  // <-- get AIM best once at entry
  startPop();
}

void Highscores::onPress(void (*goMenu)()){
  const Item &it = ITEMS[sel];
  if(it.type == IT_BACK){
    Haptics::back();
    goMenu();
    return;
  }
  if(it.type == IT_RESET){
    // Reset Pong/Snake via Storage
    Storage::resetHighscores();
    // Reset AIM high locally (AIM stores in Preferences)
    resetAimBest();
    Haptics::tap();
    return;
  }
  // selecting a game does nothing on press; score shown below
  Haptics::press();
}

static int getScoreForItem(const Item &it){
  switch(it.type){
    case IT_PONG:  return Storage::bestPong;
    case IT_SNAKE: return Storage::bestSnake;
    case IT_AIM:   return s_bestAim; // <-- NEW
    default:       return 0;
  }
}

void Highscores::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob){
  int pos = knob.get();
  int d = pos - lastKnob;
  lastKnob = pos;

  if(d){
    // carousel navigation
    Haptics::tap();
    const int step = (d>0)?1:-1;
    sel = (sel + step + IT_COUNT) % IT_COUNT;
    startPop();
  }

  spr.fillSprite(Theme::BG);

  const float t = popT();
  const int iconCX = CX;
  const int iconCY = CY - 6 - int(8 * (1.f - t));

  // draw centered card for current selection
  const Item &it = ITEMS[sel];

  // draw icon centered (if any)
  if(it.icon){
    drawIconTransparent(spr,
                        iconCX - it.w/2,
                        iconCY - it.h/2 - 6,
                        it.icon, it.w, it.h, it.colorkey);
    // name below icon
    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    spr.drawString(it.name, CX, iconCY + it.h/2 + 4); // Adjusted position
  } else {
    // No icon (Reset card): big label
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_RED, Theme::BG);
    spr.setTextSize(1);
    spr.drawString("Reset Scores", CX, iconCY - 4);
    spr.setTextSize(0);
  }

  // bottom area: show score
  if(it.type == IT_PONG || it.type == IT_SNAKE || it.type == IT_AIM){
    int score = getScoreForItem(it);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", score); // just the number, per your style
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    spr.drawString(buf, CX, SCREEN_HEIGHT - 18);
  } else if(it.type == IT_BACK){
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_WHITE, Theme::BG);
  } else if(it.type == IT_RESET){
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_RED, Theme::BG);
  }

  spr.pushSprite(0,0);
}

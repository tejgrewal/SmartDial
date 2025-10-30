#include "Highscores.h"
#include "AppConfig.h"
#include "Storage.h"
#include "Widgets.h"
#include "Haptics.h"
#include "Icons.h"
#include "Blit.h"
#include "Theme.h"

static int lastKnob = 0;
static int sel = 0; // index into ITEMS
static const int ICON_SIZE = 40;
static uint32_t s_popStartMs = 0;

enum ItemType { IT_BACK=0, IT_PONG, IT_SNAKE, IT_RESET, IT_COUNT };

struct Item {
  const char* name;
  const uint16_t* icon;
  int w, h;
  uint16_t colorkey;
  ItemType type;
};

// NOTE: Pong uses infinityIcon in this project
static const Item ITEMS[IT_COUNT] = {
  { "Back",         backIcon,     backIcon_W,     backIcon_H,     backIcon_COLORKEY,    IT_BACK  },
  { "Pong",         infinityIcon, infinityIcon_W, infinityIcon_H, infinityIcon_COLORKEY,IT_PONG  },
  { "Snake",        snakeIcon,    snakeIcon_W,    snakeIcon_H,    snakeIcon_COLORKEY,   IT_SNAKE },
  { "Reset Scores", nullptr,      0,              0,              0,                    IT_RESET }
};

static inline void startPop(){ s_popStartMs = millis(); }
static inline float popT(){
  float t = (millis() - s_popStartMs) / 180.0f;
  if (t > 1.f) t = 1.f;
  return t;
}

void Highscores::init(TFT_eSprite &, ModulinoKnob &knob){
  lastKnob = knob.get();
  sel = 0;
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
    Storage::resetHighscores();
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

  // draw icon centered
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

  // bottom area: show score or hint
  if(it.type == IT_PONG || it.type == IT_SNAKE){
    int score = getScoreForItem(it);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", score); // Changed to just show the score
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    spr.drawString(buf, CX, SCREEN_HEIGHT - 18);
  } else if(it.type == IT_BACK){
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    //spr.drawString("Press to return to menu", CX, SCREEN_HEIGHT - 18);
  } else if(it.type == IT_RESET){
    spr.setTextDatum(BC_DATUM);
    spr.setTextColor(TFT_RED, Theme::BG);
    //spr.drawString("Press to reset all high scores", CX, SCREEN_HEIGHT - 18);
  }

  // small page indicator dots
  //const int dots = IT_COUNT;
  //const int dotY = SCREEN_HEIGHT - 34;
 // const int dotX0 = CX - (dots*8)/2;
 // for(int i=0;i<dots;i++){
 //   uint16_t c = (i==sel)?TFT_WHITE:TFT_LIGHTGREY;
 //   spr.fillCircle(dotX0 + i*8, dotY, (i==sel)?3:2, c);
 // }

  spr.pushSprite(0,0);
}
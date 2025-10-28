#include "Lights.h"
#include "AppConfig.h"
#include "Theme.h"
#include "Icons.h"
#include "Blit.h"
#include "Haptics.h"
#include "BLEHost.h"

// ---------------- Types ----------------
struct Card {
  const char* name;
  const uint16_t* icon;
  uint16_t w, h, colorkey;
};

// ---------------- Icons ----------------
static Card CARD_OUTDOOR   = {"Outdoor",  lampIcon,        lampIcon_W,        lampIcon_H,        lampIcon_COLORKEY};
static Card CARD_BEDROOM   = {"Bedroom",  bedroomIcon,     bedroomIcon_W,     bedroomIcon_H,     bedroomIcon_COLORKEY};
static Card CARD_DINING    = {"Dining",   diningIcon,      diningIcon_W,      diningIcon_H,      diningIcon_COLORKEY};
static Card CARD_BACK      = {"Back",     backIcon,        backIcon_W,        backIcon_H,        backIcon_COLORKEY};

static Card CARD_OFF       = {"Off",      lampIcon,        lampIcon_W,        lampIcon_H,        lampIcon_COLORKEY};
static Card CARD_BACK2     = {"Back",     backIcon,        backIcon_W,        backIcon_H,        backIcon_COLORKEY};

static Card CARD_HALLOWEEN = {"Halloween",halloweenIcon,   halloweenIcon_W,   halloweenIcon_H,   halloweenIcon_COLORKEY};
static Card CARD_XMAS      = {"Christmas",christmasIcon,   christmasIcon_W,   christmasIcon_H,   christmasIcon_COLORKEY};
static Card CARD_NEWYEAR   = {"New Year", newYearIcon,     newYearIcon_W,     newYearIcon_H,     newYearIcon_COLORKEY};
static Card CARD_DIWALI    = {"Diwali",   diwaliIcon,      diwaliIcon_W,      diwaliIcon_H,      diwaliIcon_COLORKEY};
static Card CARD_BACK3     = {"Back",     backIcon,        backIcon_W,        backIcon_H,        backIcon_COLORKEY}; // Back inside themes

// ---------------- State machine ----------------
enum class LState { ROOT, OUTDOOR_MODE, OUTDOOR_THEME };
static LState   s_state       = LState::ROOT;
static int      s_sel         = 0;
static int      s_lastKnob    = 0;
static bool     s_wasPressed  = false;
static uint32_t s_pressStart  = 0;
static bool     s_exitFlag    = false;
static uint32_t s_popStartMs  = 0;

static const uint32_t EXIT_HOLD_MS = 650; // no longer used for nav, only for future if needed

// ---------------- Page data ----------------
static const Card* ROOT_CARDS[]          = { &CARD_OUTDOOR, &CARD_BEDROOM, &CARD_DINING, &CARD_BACK };
static const int   ROOT_COUNT            = sizeof(ROOT_CARDS)/sizeof(ROOT_CARDS[0]);
static const Card* OUTDOOR_MODE_CARDS[]  = { &CARD_OFF, &CARD_BACK2 };
static const int   OUTDOOR_MODE_COUNT    = sizeof(OUTDOOR_MODE_CARDS)/sizeof(OUTDOOR_MODE_CARDS[0]);
static const Card* OUTDOOR_THEME_CARDS[] = { &CARD_HALLOWEEN, &CARD_XMAS, &CARD_NEWYEAR, &CARD_DIWALI, &CARD_BACK3 };
static const int   OUTDOOR_THEME_COUNT   = sizeof(OUTDOOR_THEME_CARDS)/sizeof(OUTDOOR_THEME_CARDS[0]);

// ---------------- Utils ----------------
static inline void startPop(){ s_popStartMs = millis(); }
static inline float popT(){ float t = (millis() - s_popStartMs) / 180.0f; return (t>1.f)?1.f:t; }

static inline void drawCard(TFT_eSprite &spr, const Card& c){
  spr.fillSprite(Theme::BG);
  const float t = popT();
  const int iconCX = CX;
  const int iconCY = CY - 12 - int(8*(1.f - t));

  // Removed yellow outline frame
  drawIconTransparent(spr,
                      iconCX - c.w/2, iconCY - c.h/2,
                      c.icon, c.w, c.h, c.colorkey);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  spr.drawString(c.name, CX, iconCY + c.h/2 + 16);

  spr.pushSprite(0,0);
}

static inline void sendCmd(const char* msg){
  if(!BLEHost::ready()){ Serial.printf("[Lights] BLE not connected, drop: %s\n", msg); Haptics::back(); return; }
  BLEHost::send(msg);
  Serial.printf("[Lights] %s\n", msg);
  Haptics::press();
}

static inline const Card& currentCard(){
  switch(s_state){
    case LState::ROOT:          return *ROOT_CARDS[s_sel];
    case LState::OUTDOOR_MODE:  return *OUTDOOR_MODE_CARDS[s_sel];
    case LState::OUTDOOR_THEME: return *OUTDOOR_THEME_CARDS[s_sel];
  }
  return *ROOT_CARDS[0];
}

// ---------------- API ----------------
void Lights::init(TFT_eSprite &spr, ModulinoKnob &knob){
  s_state = LState::ROOT;
  s_sel = 0;
  s_lastKnob = knob.get();
  s_wasPressed = knob.isPressed();
  s_exitFlag = false;
  startPop();
  (void)spr;
}

void Lights::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob){
  const int pos = knob.get();
  const int d   = pos - s_lastKnob;
  s_lastKnob    = pos;

  int pageCount = 1;
  switch(s_state){
    case LState::ROOT:          pageCount = ROOT_COUNT; break;
    case LState::OUTDOOR_MODE:  pageCount = OUTDOOR_MODE_COUNT; break;
    case LState::OUTDOOR_THEME: pageCount = OUTDOOR_THEME_COUNT; break;
  }

  if(d != 0){
    Haptics::tap();
    const int step = (d>0)?1:-1;
    int prev = s_sel;
    s_sel = (s_sel + step + pageCount) % pageCount;
    if(s_sel != prev) startPop();
  }

  const bool nowPressed = knob.isPressed();

  // rising edge
  if(!s_wasPressed && nowPressed) s_pressStart = millis();

  // falling edge = CLICK (only forward / explicit back by selecting "Back")
  if(s_wasPressed && !nowPressed){
    switch(s_state){
      case LState::ROOT:
        if(ROOT_CARDS[s_sel] == &CARD_OUTDOOR){
          s_state = LState::OUTDOOR_MODE; s_sel = 0; startPop();
        } else if(ROOT_CARDS[s_sel] == &CARD_BACK){
          s_exitFlag = true; Haptics::back();
        } else {
          Haptics::tap();
          Serial.println("[Lights] Coming soon");
        }
        break;

      case LState::OUTDOOR_MODE:
        if(OUTDOOR_MODE_CARDS[s_sel] == &CARD_OFF){
          sendCmd("LIGHT:OUTDOOR:OFF");
          s_state = LState::OUTDOOR_THEME; s_sel = 0; startPop();
        } else { // Back
          s_state = LState::ROOT; s_sel = 0; Haptics::back(); startPop();
        }
        break;

      case LState::OUTDOOR_THEME:
        if(OUTDOOR_THEME_CARDS[s_sel] == &CARD_BACK3){
          s_state = LState::OUTDOOR_MODE; s_sel = 0; Haptics::back(); startPop();
        } else {
          if(OUTDOOR_THEME_CARDS[s_sel] == &CARD_HALLOWEEN)      sendCmd("LIGHT:OUTDOOR:THEME:HALLOWEEN");
          else if(OUTDOOR_THEME_CARDS[s_sel] == &CARD_XMAS)      sendCmd("LIGHT:OUTDOOR:THEME:CHRISTMAS");
          else if(OUTDOOR_THEME_CARDS[s_sel] == &CARD_NEWYEAR)   sendCmd("LIGHT:OUTDOOR:THEME:NEWYEAR");
          else if(OUTDOOR_THEME_CARDS[s_sel] == &CARD_DIWALI)    sendCmd("LIGHT:OUTDOOR:THEME:DIWALI");
        }
        break;
    }
  }

  // Important: NO long-press navigation or exit here anymore.
  // If your framework globally maps long-press to "exit app",
  // disable that while Lights is active.

  s_wasPressed = nowPressed;
  drawCard(spr, currentCard());
}

bool Lights::wantsExit(){
  bool v = s_exitFlag; s_exitFlag = false; return v;
}

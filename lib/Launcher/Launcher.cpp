#include "Launcher.h"
#include "AppConfig.h"
#include "Theme.h"
#include "Icons.h"
#include "Blit.h"
#include "Haptics.h"
#include "BLEHost.h"

// ------------ App list ------------
struct App {
  const char* name;
  const uint16_t* icon;
  uint16_t w, h, colorkey;
  const char* run;
};

static App APPS[] = {
  {"Spotify",      musicIcon,      musicIcon_W,      musicIcon_H,      musicIcon_COLORKEY,      "spotify"},
  {"Photoshop",    photoshopIcon,  photoshopIcon_W,  photoshopIcon_H,  photoshopIcon_COLORKEY,  "photoshop"},
  {"Altium",       altiumIcon,     altiumIcon_W,     altiumIcon_H,     altiumIcon_COLORKEY,     "altium"},
  {"Chrome",       chromeIcon,     chromeIcon_W,     chromeIcon_H,     chromeIcon_COLORKEY,     "chrome"},
  {"VS Code",      vsCodeIcon,     vsCodeIcon_W,     vsCodeIcon_H,     vsCodeIcon_COLORKEY,     "code"},
  {"Arduino IDE",  arduinoIcon,    arduinoIcon_W,    arduinoIcon_H,    arduinoIcon_COLORKEY,    "arduino"},
};
static const int APP_COUNT = sizeof(APPS)/sizeof(APPS[0]);

// ------------ State ------------
static int       s_sel        = 0;
static int       s_lastKnob   = 0;
static bool      s_wasPressed = false;
static uint32_t  s_pressStart = 0;
static bool      s_exitFlag   = false;
static uint32_t  s_popStartMs = 0;

static const uint32_t EXIT_HOLD_MS = 650;

// ------------ Helpers ------------
static inline void startPop(){ s_popStartMs = millis(); }
static inline float popT(){
  float t = (millis() - s_popStartMs) / 180.0f;
  if (t > 1.f) t = 1.f;
  return t;
}

static inline void drawSingleCard(TFT_eSprite &spr) {
  spr.fillSprite(Theme::BG);

  const float t = popT();
  const int iconCX = CX;
  const int iconCY = CY - 12 - int(8 * (1.f - t));
  const int framePad = 10;

  // selection frame
  spr.drawRoundRect(iconCX - APPS[s_sel].w/2 - framePad,
                    iconCY - APPS[s_sel].h/2 - framePad,
                    APPS[s_sel].w + framePad*2,
                    APPS[s_sel].h + framePad*2,
                    12, TFT_BLACK);

  // icon
  drawIconTransparent(spr,
                      iconCX - APPS[s_sel].w/2,
                      iconCY - APPS[s_sel].h/2,
                      APPS[s_sel].icon, APPS[s_sel].w, APPS[s_sel].h,
                      APPS[s_sel].colorkey);

  // label below icon
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  spr.drawString(APPS[s_sel].name, CX, iconCY + APPS[s_sel].h/2 + 16);

  spr.pushSprite(0, 0);
}

static inline void sendRunCommand() {
  if (!BLEHost::ready()) {
    Serial.println("[Launcher] BLE not connected");
    Haptics::back();
    return;
  }
  char buf[64];
  snprintf(buf, sizeof(buf), "RUN:%s", APPS[s_sel].run);
  BLEHost::send(buf);
  Serial.printf("[Launcher] Sent %s\n", buf);
  Haptics::press();
}

// ------------ API ------------
void Launcher::init(TFT_eSprite &spr, ModulinoKnob &knob) {
  s_sel        = 0;
  s_lastKnob   = knob.get();
  s_wasPressed = knob.isPressed();
  s_exitFlag   = false;
  startPop();
  (void)spr;
}

void Launcher::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob) {
  const int pos = knob.get();
  const int d   = pos - s_lastKnob;
  s_lastKnob    = pos;

  if (d != 0) {
    Haptics::tap();
    const int step = (d > 0) ? 1 : -1;
    const int prev = s_sel;
    s_sel = (s_sel + step + APP_COUNT) % APP_COUNT;
    if (s_sel != prev) startPop();
  }

  const bool nowPressed = knob.isPressed();
  if (!s_wasPressed && nowPressed) s_pressStart = millis();
  if (s_wasPressed && !nowPressed) {
    if (millis() - s_pressStart < EXIT_HOLD_MS) sendRunCommand();
  }
  if (nowPressed && (millis() - s_pressStart >= EXIT_HOLD_MS) && !s_exitFlag) {
    s_exitFlag = true;
    Haptics::back();
  }
  s_wasPressed = nowPressed;

  drawSingleCard(spr);
}

bool Launcher::wantsExit() {
  const bool v = s_exitFlag;
  s_exitFlag = false;
  return v;
}

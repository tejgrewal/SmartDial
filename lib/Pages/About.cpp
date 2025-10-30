#include "Highscores.h"
#include "AppConfig.h"
#include "Storage.h"
#include "Widgets.h"
#include "Haptics.h"
#include "Icons.h"
#include "Blit.h"
#include "Theme.h"
#include "About.h"
#include "Menu.h"


void About::init(TFT_eSprite &, ModulinoKnob &){
  // nothing to initialize
}

// --- Add missing draw() implementation required by main.cpp ---
void About::draw(TFT_eSprite &spr){
  spr.fillSprite(TFT_BLACK);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("Smart Dial", CX, 84);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(0xC618, TFT_BLACK);
  const int y = 108, line = 18;
  spr.drawString("\x95 Created By", CX, y + 0*line);
  spr.drawString("\x95 Tej Grewal", CX, y + 1*line);

  spr.pushSprite(0,0);
}
// --- end draw() ---

void About::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &){
  spr.fillSprite(TFT_BLACK);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("Smart Dial", CX, 84);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(0xC618, TFT_BLACK);
  const int y = 108, line = 18;
  spr.drawString("\x95 Created By", CX, y + 0*line);
  spr.drawString("\x95 Tej Grewal", CX, y + 1*line);

  spr.pushSprite(0,0);
}

// Pressing anywhere while About is active returns to main menu
void About::onPress(void (*goMenu)()){
  Haptics::back();
  goMenu();
}
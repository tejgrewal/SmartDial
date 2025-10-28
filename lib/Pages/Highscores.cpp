#include "Highscores.h"
#include "AppConfig.h"
#include "Storage.h"
#include "Widgets.h"
#include "Haptics.h"
static int lastKnob=0; static int sel=0; // 0=Back,1=Reset
void Highscores::init(TFT_eSprite &, ModulinoKnob &knob){ lastKnob=knob.get(); }
void Highscores::onPress(void (*goMenu)()){ if(sel==0){ Haptics::back(); goMenu(); } else { Storage::resetHighscores(); Haptics::tap(); } }
void Highscores::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob){ int pos=knob.get(), d=pos-lastKnob; lastKnob=pos; if(d) { sel=1-sel; Haptics::tap(); }
spr.fillSprite(TFT_BLACK);
spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_YELLOW,TFT_BLACK); spr.drawString("High Scores", CX, 62);
spr.setTextColor(TFT_WHITE,TFT_BLACK); spr.drawString("Infinity Pong", CX, 96);
char b1[16]; snprintf(b1,sizeof(b1),"%d", Storage::bestPong); spr.drawString(b1, CX, 114);
spr.drawString("Snake", CX, 140); char b2[16]; snprintf(b2,sizeof(b2),"%d", Storage::bestSnake); spr.drawString(b2, CX, 158);
Widgets::drawChoiceModal(spr, "Back", "Reset Scores", sel); spr.pushSprite(0,0);
}
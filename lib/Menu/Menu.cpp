#include "Menu.h"
#include "Theme.h"
#include "Icons.h"
#include "Blit.h"
#include "Haptics.h" 


struct Item{ const char* label; MenuId id; };
static Item ITEMS[]={{"Home",MenuId::M_HOME},{"Launcher", MenuId::M_LAUNCHER},{"Lights",MenuId::M_BULB},{"Torch",MenuId::M_TORCH},{"Infinity Pong",MenuId::M_PONG},{"Snake",MenuId::M_SNAKE},{"Maze",MenuId::M_MAZE},{"High Scores",MenuId::M_HISCORES},{"About",MenuId::M_ABOUT}};
static const int COUNT = sizeof(ITEMS)/sizeof(ITEMS[0]);
static int menuIndex=0; static int lastKnob=0; static uint32_t popStart=0;


void Menu::init(TFT_eSprite &spr, ModulinoKnob &knob){ lastKnob=knob.get(); }
void Menu::startPop(){ popStart=millis(); }
static float popT(){ float t=(millis()-popStart)/180.0f; return t>1?1:t; }
MenuId Menu::currentId(){ return ITEMS[menuIndex].id; } 


static void draw_label(TFT_eSprite &spr, const char* txt){
spr.setFreeFont(&FreeSansBold24pt); spr.setTextDatum(TC_DATUM);
spr.setTextColor(TFT_WHITE, Theme::BG);
int y = CY + 120/2 + 8; if(y>SCREEN_HEIGHT-4) y = SCREEN_HEIGHT-4; spr.drawString(txt, CX, y);
}


void Menu::tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob){
int pos=knob.get(), d=pos-lastKnob; if(d!=0){ Haptics::tap(); int prev=menuIndex; if(d>0) menuIndex=(menuIndex+1)%COUNT; else menuIndex=(menuIndex-1+COUNT)%COUNT; if(menuIndex!=prev) startPop(); lastKnob=pos; }
spr.fillSprite(Theme::BG);
const auto &it = ITEMS[menuIndex]; float t = popT(); const int iconCX=CX; const int iconCY = CY - 18 - int(8*(1.0f-t));
switch(it.id){
case MenuId::M_HOME: drawIconTransparent(spr, iconCX-60, iconCY-60, homeIcon, homeIcon_W, homeIcon_H, homeIcon_COLORKEY); break;
case MenuId::M_BULB: drawIconTransparent(spr, iconCX-60, iconCY-60, bulbIcon, bulbIcon_W, bulbIcon_H, bulbIcon_COLORKEY); break;
case MenuId::M_LAUNCHER:drawIconTransparent(spr, iconCX-60, iconCY-60,keyboardShortcutIcon, keyboardShortcutIcon_W, keyboardShortcutIcon_H,keyboardShortcutIcon_COLORKEY); break;
case MenuId::M_TORCH: drawIconTransparent(spr, iconCX-60, iconCY-60, torchIcon, torchIcon_W, torchIcon_H, torchIcon_COLORKEY); break;
case MenuId::M_PONG: drawIconTransparent(spr, iconCX-60, iconCY-60, infinityIcon, infinityIcon_W, infinityIcon_H, infinityIcon_COLORKEY); break;
case MenuId::M_SNAKE: drawIconTransparent(spr, iconCX-60, iconCY-60, snakeIcon, snakeIcon_W, snakeIcon_H, snakeIcon_COLORKEY); break;
case MenuId::M_MAZE: drawIconOpaque(spr, iconCX-60, iconCY-60, mazeIcon, mazeIcon_W, mazeIcon_H); break;
case MenuId::M_HISCORES: drawIconTransparent(spr, iconCX-60, iconCY-60, highScoreIcon, highScoreIcon_W, highScoreIcon_H, highScoreIcon_COLORKEY); break;
case MenuId::M_ABOUT: drawIconTransparent(spr, iconCX-60, iconCY-60, infoIcon, infoIcon_W, infoIcon_H, infoIcon_COLORKEY); break;
}
draw_label(spr, it.label); spr.pushSprite(0,0);
}
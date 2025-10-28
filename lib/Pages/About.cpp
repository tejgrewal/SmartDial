#include "About.h"
#include "AppConfig.h"
#include "Icons.h"
#include "Blit.h"
void About::init(TFT_eSprite &, ModulinoKnob &){}
void About::draw(TFT_eSprite &spr){
spr.fillSprite(TFT_BLACK);
drawIconTransparent(spr, CX-60, 54-60, infoIcon, infoIcon_W, infoIcon_H, infoIcon_COLORKEY);
spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_WHITE,TFT_BLACK);
spr.drawString("Game Hub", CX, 100);
spr.setTextColor(0xC618,TFT_BLACK);
const int y=128, line=18;
spr.drawString("\x95 Rotate: scroll/turn", CX, y+0*line);
spr.drawString("\x95 Press: select/back", CX, y+1*line);
spr.drawString("\x95 Snake: safer turning", CX, y+2*line);
spr.drawString("\x95 Pong: hidden rim", CX, y+3*line);
spr.pushSprite(0,0);
}
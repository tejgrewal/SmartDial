#pragma once
#include <TFT_eSPI.h>
#include "AppConfig.h"
#include "Theme.h"
#include "FreeSansBold24pt.h"


namespace Widgets {
inline float deg2rad(float d){ return d*3.14159265f/180.0f; }
inline float rad2deg(float r){ return r*180.0f/3.14159265f; }
inline float norm_deg(float d){ while(d<0)d+=360.0f; while(d>=360.0f)d-=360.0f; return d; }


inline void drawChoiceModal(TFT_eSprite &sprite, const char* opt0, const char* opt1, int sel){
const int w=160, h=96, x=CX-w/2, y=CY-h/2;
sprite.fillRoundRect(x, y, w, h, 10, TFT_BLACK);
sprite.drawRoundRect(x, y, w, h, 10, TFT_DARKGREY);
const int bw=w-24, bh=32, bx=x+12; const int by0=y+14, by1=y+14+bh+10;
uint16_t c0 = (sel==0)?TFT_YELLOW:TFT_WHITE;
sprite.drawRoundRect(bx, by0, bw, bh, 8, c0);
sprite.setTextDatum(MC_DATUM); sprite.setTextColor(c0, TFT_BLACK);
sprite.drawString(opt0, bx+bw/2, by0+bh/2);
uint16_t c1 = (sel==1)?TFT_YELLOW:TFT_WHITE;
sprite.drawRoundRect(bx, by1, bw, bh, 8, c1);
sprite.setTextColor(c1, TFT_BLACK); sprite.drawString(opt1, bx+bw/2, by1+bh/2);
}


inline void drawThickArc(TFT_eSprite &sprite,int cx,int cy,float rOuter,int thick,float sD,float eD,uint16_t c){
auto seg=[&](float a0,float a1){
const float ri=rOuter-thick; float a0r=deg2rad(a0), a1r=deg2rad(a1);
int x0o=cx+roundf(rOuter*cosf(a0r)), y0o=cy+roundf(rOuter*sinf(a0r));
int x1o=cx+roundf(rOuter*cosf(a1r)), y1o=cy+roundf(rOuter*sinf(a1r));
int x0i=cx+roundf(ri*cosf(a0r)), y0i=cy+roundf(ri*sinf(a0r));
int x1i=cx+roundf(ri*cosf(a1r)), y1i=cy+roundf(ri*sinf(a1r));
sprite.fillTriangle(x0o,y0o,x1o,y1o,x0i,y0i,c);
sprite.fillTriangle(x0i,y0i,x1o,y1o,x1i,y1i,c);
};
sD=norm_deg(sD); eD=norm_deg(eD); const float STEP=2.0f;
if(sD<=eD){ for(float a=sD;a<eD;a+=STEP){ float b=(a+STEP<eD)?a+STEP:eD; seg(a,b);} }
else{ for(float a=sD;a<360.0f;a+=STEP){ float b=(a+STEP<360.0f)?a+STEP:360.0f; seg(a,b);} for(float a=0;a<eD;a+=STEP){ float b=(a+STEP<eD)?a+STEP:eD; seg(a,b);} }
}
}
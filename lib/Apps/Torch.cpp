#include "Torch.h"

static inline uint16_t rgb565(uint8_t r,uint8_t g,uint8_t b){ return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
static const int W=60,H=120; static uint8_t buf[W*H]; static uint16_t pal[64]; static uint32_t seed=1;
static inline uint32_t trand(){ seed=1664525UL*seed+1013904223UL; return seed; }
static void buildPal(){ for(int i=0;i<64;i++){ float t=i/63.0f; float r=255.0f*(0.6f+0.4f*t), g=255.0f*(0.3f+0.7f*t), b=40.0f*t; if(r>255)r=255; if(g>255)g=255; if(b>255)b=255; pal[i]=rgb565((uint8_t)r,(uint8_t)g,(uint8_t)b);} }
void Torch::initOnce(TFT_eSprite &){ buildPal(); }
void Torch::init(){ memset(buf,0,sizeof(buf)); seed = millis() ^ 0x5A5A5A5A; }
void Torch::step(){ for(int x=0;x<W;x++){ uint8_t r=(trand()>>24)&0xFF; buf[(H-1)*W+x]=48+(r>>2); buf[(H-2)*W+x]=40+(r>>3); buf[(H-3)*W+x]=30+(r>>3);} int wind=((trand()>>29)&0x3)-1; for(int y=1;y<H;y++){ int srcY=H-y, dstY=H-y-1; for(int x=0;x<W;x++){ int x0=x+wind-1; if(x0<0)x0=0; if(x0>=W)x0=W-1; int x1=x+wind; if(x1<0)x1=0; if(x1>=W)x1=W-1; int x2=x+wind+1; if(x2<0)x2=0; if(x2>=W)x2=W-1; int a=buf[srcY*W+x0], b=buf[srcY*W+x1], c=buf[srcY*W+x2]; int m=(a+b+c)/3; m-=((trand()>>27)&0x7); if(m<0)m=0; if(m>63)m=63; buf[dstY*W+x]=(uint8_t)m; } } }
void Torch::render(TFT_eSprite &spr){ spr.fillSprite(TFT_BLACK); const int sx=4, sy=2; for(int y=0;y<H;y++){ int yy=y*sy; for(int x=0;x<W;x++){ spr.fillRect(x*sx,yy,sx,sy, pal[ buf[y*W+x] ] ); } } spr.pushSprite(0,0); }
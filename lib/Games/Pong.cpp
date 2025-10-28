#include "Pong.h"
#include "AppConfig.h"
#include "Theme.h"
#include "Widgets.h"
#include "Haptics.h"
#include "Storage.h"


using namespace Widgets;


static ModulinoKnob* k=nullptr; static int lastKnobPos=0;
static const float R_FIELD=110.0f, R_PADDLE=104.0f; static const int PADDLE_THICK=10, BALL_R=8;
static const float PADDLE_SPAN_BASE=60.0f, PADDLE_SPAN_DEC=8.0f, PADDLE_SPAN_MIN=20.0f;
static const float DEG_PER_TICK=9.0f, BOUNCE_DAMP=0.85f, BUMP_SPIN=0.6f, SPEED_SCALE=0.5f;
static const float BASE_SPEED=2.6f, SPEED_PER_10=0.18f, LEVEL_SPEED_UP=0.06f, SPEED_CLAMP=7.5f; static const int LEVEL_POINTS=50;
static float ball_x=CX, ball_y=CY-30, ball_vx=1.6f, ball_vy=2.1f, paddleDeg=0.0f; static int score=0, level=1; static uint32_t runStartMs=0; static const uint32_t START_GRACE_MS=1500;


enum class GState{ RUNNING, GAME_OVER, COUNTDOWN, PAUSED }; static GState gs=GState::COUNTDOWN; static int countdownVal=3; static uint32_t countdownTimer=0; static bool levelFlash=false; static uint32_t levelFlashUntil=0;


static float angle_of(float x,float y){ float a=atan2f(y-CY,x-CX); float d=rad2deg(a); if(d<0)d+=360.0f; return d; }
static float shortest_deg_delta(float a,float b){ return fmodf(a-b+540.0f,360.0f)-180.0f; }
static void reflect(float nx,float ny,float &vx,float &vy){ float dot=vx*nx+vy*ny; vx-=2.0f*dot*nx; vy-=2.0f*dot*ny; }
static void set_speed_mag(float target){ float sp=sqrtf(ball_vx*ball_vx+ball_vy*ball_vy); if(sp<1e-3f) return; float s=target/sp; ball_vx*=s; ball_vy*=s; }
static float span(){ float s=PADDLE_SPAN_BASE-(level-1)*PADDLE_SPAN_DEC; return (s<PADDLE_SPAN_MIN)?PADDLE_SPAN_MIN:s; }
static float target_speed(){ float t=BASE_SPEED+(score/10)*SPEED_PER_10+(level-1)*LEVEL_SPEED_UP; if(t>SPEED_CLAMP)t=SPEED_CLAMP; return t*SPEED_SCALE; }
static void launch_ball(){ float th=deg2rad(paddleDeg); float nx=cosf(th), ny=sinf(th); ball_x=CX; ball_y=CY; float ts=target_speed(); if(ts<0.8f) ts=0.8f; ball_vx=nx*ts; ball_vy=ny*ts; float tg=deg2rad(paddleDeg+90.0f); ball_vx += 0.12f*cosf(tg); ball_vy += 0.12f*sinf(tg); }


static void drawHUD(TFT_eSprite &spr){ spr.setTextDatum(BC_DATUM); spr.setTextColor(TFT_YELLOW, Theme::BG); char b[24]; snprintf(b,sizeof(b),"SCORE %d",score); spr.drawString(b,CX,SCREEN_HEIGHT-6); }
static void drawGO(TFT_eSprite &spr){ spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_WHITE,Theme::BG); spr.drawString("GAME OVER", CX, CY-18); spr.setTextColor(TFT_YELLOW,Theme::BG); char b[24]; snprintf(b,sizeof(b),"SCORE %d",score); spr.drawString(b, CX, CY+6); spr.setTextColor(TFT_CYAN,Theme::BG); spr.drawString("Press to return", CX, CY+30); }
static void drawCountdown(TFT_eSprite &spr){ spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_WHITE,Theme::BG); spr.setTextSize(5); char buf[8]; sprintf(buf,"%d",countdownVal); spr.drawString(buf,CX,CY); spr.setTextSize(1); }
static void drawLevelFlash(TFT_eSprite &spr){ if(levelFlash && millis()<levelFlashUntil){ spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_GREEN,Theme::BG); char buf[16]; snprintf(buf,sizeof(buf),"LEVEL %d",level); spr.drawString(buf, CX, CY-40); } else levelFlash=false; }


void Pong::init(TFT_eSprite &, ModulinoKnob &knob){ k=&knob; lastKnobPos=knob.get(); }
void Pong::reset(){ score=0; level=1; paddleDeg=0.0f; ball_x=CX; ball_y=CY; ball_vx=ball_vy=0; countdownVal=3; countdownTimer=millis(); gs=GState::COUNTDOWN; levelFlash=false; }
static void updateLevel(){ int nl=1+(score/LEVEL_POINTS); if(nl!=level){ level=nl; levelFlash=true; levelFlashUntil=millis()+900; set_speed_mag(target_speed()); } }
static void updatePaddle(){ int pos=k->get(), d=pos-lastKnobPos; lastKnobPos=pos; if(d>6) d=6; else if(d<-6) d=-6; paddleDeg = norm_deg(paddleDeg + d*DEG_PER_TICK); }


void Pong::tick(float dt){ if(gs!=GState::RUNNING) return; ball_x += ball_vx*dt; ball_y += ball_vy*dt; float t=target_speed(), sp=sqrtf(ball_vx*ball_vx+ball_vy*ball_vy); if(sp>1e-3f){ float adj=t/sp; if(adj<0.5f)adj=0.5f; if(adj>1.5f)adj=1.5f; ball_vx*=0.985f+0.015f*adj; ball_vy*=0.985f+0.015f*adj; }
float dx=ball_x-CX, dy=ball_y-CY; float r=sqrtf(dx*dx+dy*dy); float ang=angle_of(ball_x,ball_y); float maxr=R_FIELD-BALL_R;
if(r>=maxr){ float aDelta=shortest_deg_delta(ang,paddleDeg); float tol=rad2deg(atan2f(BALL_R,R_FIELD)); bool within=fabsf(aDelta)<= (span()*0.5f + tol); bool movingOut=(dx*ball_vx+dy*ball_vy)>0; if(within && movingOut){ float nx=dx/(r+1e-3f), ny=dy/(r+1e-3f); ball_x= CX+nx*(maxr-0.5f); ball_y= CY+ny*(maxr-0.5f); reflect(nx,ny,ball_vx,ball_vy); ball_vx*=BOUNCE_DAMP; ball_vy*=BOUNCE_DAMP; float spin=aDelta/(span()*0.5f), tg=deg2rad(ang+90.0f); ball_vx += BUMP_SPIN*spin*cosf(tg); ball_vy += BUMP_SPIN*spin*sinf(tg); score+=10; set_speed_mag(target_speed()); updateLevel(); } else { if(millis()-runStartMs<START_GRACE_MS){ float nx=dx/(r+1e-3f), ny=dy/(r+1e-3f); ball_x= CX+nx*(maxr-0.5f); ball_y= CY+ny*(maxr-0.5f); reflect(nx,ny,ball_vx,ball_vy); ball_vx*=0.90f; ball_vy*=0.90f; } else { gs=GState::GAME_OVER; Storage::savePong(score); } } }
}


void Pong::draw(TFT_eSprite &spr){
spr.fillSprite(Theme::BG);
Widgets::drawThickArc(spr, CX, CY, R_PADDLE, PADDLE_THICK, paddleDeg-span()*0.5f, paddleDeg+span()*0.5f, Theme::current());
spr.fillCircle(roundf(ball_x),roundf(ball_y),BALL_R,TFT_WHITE);
if(gs==GState::RUNNING){ drawHUD(spr); drawLevelFlash(spr);} else if(gs==GState::GAME_OVER){ drawGO(spr);} else if(gs==GState::COUNTDOWN){ drawCountdown(spr);} else if(gs==GState::PAUSED){ spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_CYAN,Theme::BG); spr.drawString("PAUSED",CX,CY);} spr.pushSprite(0,0);
}


void Pong::onPress(void (*goMenu)()){ static bool showMenu=false; static int sel=0; if(showMenu){ if(sel==0){ reset(); showMenu=false; } else { showMenu=false; Haptics::back(); goMenu(); } return; }
if(gs==GState::COUNTDOWN){ if(millis()-countdownTimer>1000){ countdownTimer=millis(); if(--countdownVal<=0){ runStartMs=millis(); launch_ball(); set_speed_mag(target_speed()); gs=GState::RUNNING; } } return; }
if(gs==GState::RUNNING){ gs=GState::PAUSED; sel=0; showMenu=true; Haptics::tap(); }
else if(gs==GState::GAME_OVER){ showMenu=true; sel=0; }
}
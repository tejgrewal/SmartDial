#include "Maze.h"
#include "AppConfig.h"
#include "Widgets.h"
#include "BNO.h"
#include "Haptics.h"
using namespace Widgets;
static const int LAB_OUTER_R=104, LAB_RING_THICK=4, LAB_SPACING=16, LAB_N_RINGS=5, MZ_BALL_R=5, GOAL_R=12; static const float ACC_SCALE=55.0f, DRAG=0.90f, MAX_V=140.0f;
static float ringR[LAB_N_RINGS]; struct Gap{ float a0,a1; }; struct RingGaps{ int count; Gap g[2]; };
static RingGaps gaps[LAB_N_RINGS]={{1,{{35,65}}},{1,{{195,225}}},{1,{{100,130}}},{1,{{288,318}}},{1,{{170,200}}}};
static inline float angNorm(float d){ while(d<0)d+=360.0f; while(d>=360.0f)d-=360.0f; return d; }
static inline bool angleInSpan(float a,float s,float e){ a=angNorm(a); s=angNorm(s); e=angNorm(e); return (s<=e)?(a>=s&&a<=e):(a>=s||a<=e); }
static float x=CX,y=CY,vx=0,vy=0; static bool win=false; static int lastKnob=0; static int menuSel=0; static bool showMenu=false;
static void build(){ for(int i=0;i<LAB_N_RINGS;i++) ringR[i]=LAB_OUTER_R - i*LAB_SPACING; }
static void placeStart(){ float sa=(gaps[0].g[0].a0+gaps[0].g[0].a1)*0.5f; float rs = ringR[0]-LAB_SPACING*0.5f; float th=deg2rad(sa); x=CX+rs*cosf(th); y=CY+rs*sinf(th); vx=vy=0; win=false; }
void Maze::init(TFT_eSprite &, ModulinoKnob &knob){ lastKnob=knob.get(); }
void Maze::reset(){ build(); placeStart(); }
static inline void reflect2(float nx,float ny,float &vx,float &vy){ float dot=vx*nx+vy*ny; vx-=2.0f*dot*nx; vy-=2.0f*dot*ny; vx*=0.75f; vy*=0.75f; }
static bool isGap(int rIdx,float ang){ const RingGaps &rg=gaps[rIdx]; for(int i=0;i<rg.count;i++) if(angleInSpan(ang,rg.g[i].a0,rg.g[i].a1)) return true; return false; }
static void collide(float &px,float &py,float &vx,float &vy){ float dx=px-CX, dy=py-CY; float r=sqrtf(dx*dx+dy*dy); float ang=rad2deg(atan2f(dy,dx)); if(ang<0) ang+=360.0f; float outer = LAB_OUTER_R - (LAB_RING_THICK/2.0f) - 1; if(r + MZ_BALL_R > outer){ float nx=dx/(r+1e-6f), ny=dy/(r+1e-6f); float target=outer - MZ_BALL_R; px=CX+nx*target; py=CY+ny*target; reflect2(nx,ny,vx,vy); return; }
for(int i=0;i<LAB_N_RINGS;i++){ float rr=ringR[i]; float dist=r-rr; if(fabsf(dist) <= (LAB_RING_THICK/2.0f + MZ_BALL_R)){ if(!isGap(i,ang)){ float nx=dx/(r+1e-6f), ny=dy/(r+1e-6f); float dir=(dist>=0)?1.0f:-1.0f; float target= rr + dir*(LAB_RING_THICK/2.0f + MZ_BALL_R + 0.5f); px=CX+nx*target; py=CY+ny*target; reflect2(nx*dir,ny*dir,vx,vy); return; } } }
if(r <= GOAL_R) win=true; }
void Maze::tick(float dt){ if(!Sensors::BNO::ready()) return; float ax,ay; Sensors::BNO::read(ax,ay); vx += (-ax)*ACC_SCALE*dt; vy += (-ay)*ACC_SCALE*dt; float sp2=vx*vx+vy*vy; if(sp2>MAX_V*MAX_V){ float s=MAX_V/sqrtf(sp2); vx*=s; vy*=s; } vx*=DRAG; vy*=DRAG; x+=vx*dt; y+=vy*dt; collide(x,y,vx,vy); }
void Maze::draw(TFT_eSprite &spr){ spr.fillSprite(TFT_BLACK); uint16_t wallC=TFT_WHITE; for(int i=0;i<LAB_N_RINGS;i++){ float cuts[6]; int cutN=0; for(int g=0; g<gaps[i].count; ++g){ float a0=angNorm(gaps[i].g[g].a0), a1=angNorm(gaps[i].g[g].a1); if(a0<=a1){ cuts[cutN++]=a0; cuts[cutN++]=a1; } else { cuts[cutN++]=a0; cuts[cutN++]=360.0f; cuts[cutN++]=0.0f; cuts[cutN++]=a1; } }
for(int a=0;a<cutN;a++) for(int b=a+1;b<cutN;b++) if(cuts[b]<cuts[a]){ float t=cuts[a]; cuts[a]=cuts[b]; cuts[b]=t; }
float prev=0.0f; float seg[6][2]; int n=0; for(int c=0;c<cutN;c+=2){ float s=cuts[c], e=cuts[c+1]; if(s>prev){ seg[n][0]=prev; seg[n][1]=s; n++; } prev=e; } if(prev<360.0f){ seg[n][0]=prev; seg[n][1]=360.0f; n++; }
for(int s=0;s<n;s++) Widgets::drawThickArc(spr, CX,CY, ringR[i], LAB_RING_THICK, seg[s][0], seg[s][1], wallC);
}
spr.drawCircle(CX,CY,GOAL_R,TFT_GREEN); spr.fillCircle((int)roundf(x),(int)roundf(y), MZ_BALL_R, TFT_YELLOW); if(showMenu) Widgets::drawChoiceModal(spr, "Replay", "Exit", menuSel); spr.pushSprite(0,0); }
void Maze::onPress(void (*goMenu)()){ if(showMenu){ if(menuSel==0){ reset(); showMenu=false; } else { showMenu=false; Haptics::back(); goMenu(); } } else { menuSel=0; showMenu=true; Haptics::tap(); } }
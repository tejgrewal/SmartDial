#include "Maze.h"
#include "AppConfig.h"
#include "Widgets.h"
#include "Theme.h"
#include "BNO.h"
#include "Haptics.h"

using namespace Widgets;

// -------- Labyrinth tuning --------
static const int   LAB_OUTER_R     = 104;
static const int   LAB_RING_THICK  = 4;
static const int   LAB_SPACING     = 16;
static const int   LAB_N_RINGS     = 5;
static const int   MZ_BALL_R       = 5;
static const int   GOAL_R          = 12;

// Motion tuning
static const float ACC_SCALE       = 55.0f;
static const float DRAG            = 0.90f;
static const float MAX_V           = 140.0f;

// ---- Maze geometry ----
static float ringR[LAB_N_RINGS];
struct Gap { float a0, a1; };
struct RingGaps { int count; Gap g[2]; };

static RingGaps gaps[LAB_N_RINGS] = {
  {1, {{ 35,  65}}},
  {1, {{195, 225}}},
  {1, {{100, 130}}},
  {1, {{288, 318}}},
  {1, {{170, 200}}}
};

// ---- Game state ----
static inline float angNorm(float d){ while(d<0)d+=360.0f; while(d>=360.0f)d-=360.0f; return d; }
static inline bool angleInSpan(float a,float s,float e){
  a=angNorm(a); s=angNorm(s); e=angNorm(e);
  return (s<=e) ? (a>=s && a<=e) : (a>=s || a<=e);
}

static float x = CX, y = CY, vx = 0, vy = 0;
static bool  win = false;

enum class GState { RUNNING, PAUSED, GAME_OVER };
static GState gs = GState::RUNNING;

static bool showMenu = false;
static int  sel = 0;
static int  lastKnob = 0;
static int  menuCount = 0;

// keep a pointer to the app knob (fixes undefined reference)
static ModulinoKnob* kptr = nullptr;

static void build(){
  for (int i=0;i<LAB_N_RINGS;i++) ringR[i] = LAB_OUTER_R - i * LAB_SPACING;
}
static void placeStart(){
  float sa = (gaps[0].g[0].a0 + gaps[0].g[0].a1) * 0.5f;
  float rs = ringR[0] - LAB_SPACING * 0.5f;
  float th = deg2rad(sa);
  x = CX + rs * cosf(th);
  y = CY + rs * sinf(th);
  vx = vy = 0;
  win = false;
  gs = GState::RUNNING;
  showMenu = false;
  sel = 0;
  menuCount = 0;
}

void Maze::init(TFT_eSprite &, ModulinoKnob &knob){
  kptr = &knob;
  lastKnob = knob.get();
  build();
  placeStart();
}
void Maze::reset(){ build(); placeStart(); }

static inline void reflect2(float nx,float ny,float &ivx,float &ivy){
  float dot = ivx*nx + ivy*ny;
  ivx -= 2.0f * dot * nx;
  ivy -= 2.0f * dot * ny;
  ivx *= 0.75f; ivy *= 0.75f;
}
static bool isGap(int rIdx,float ang){
  const RingGaps &rg = gaps[rIdx];
  for(int i=0;i<rg.count;i++)
    if (angleInSpan(ang, rg.g[i].a0, rg.g[i].a1)) return true;
  return false;
}
static void collide(float &px,float &py,float &ivx,float &ivy){
  float dx = px - CX, dy = py - CY;
  float r  = sqrtf(dx*dx + dy*dy);
  float ang = rad2deg(atan2f(dy,dx)); if(ang<0) ang += 360.0f;

  float outer = LAB_OUTER_R - (LAB_RING_THICK/2.0f) - 1;
  if (r + MZ_BALL_R > outer){
    float nx = dx/(r+1e-6f), ny = dy/(r+1e-6f);
    float target = outer - MZ_BALL_R;
    px = CX + nx*target; py = CY + ny*target;
    reflect2(nx,ny,ivx,ivy);
    return;
  }

  for(int i=0;i<LAB_N_RINGS;i++){
    float rr = ringR[i];
    float dist = r - rr;
    if (fabsf(dist) <= (LAB_RING_THICK/2.0f + MZ_BALL_R)){
      if (!isGap(i, ang)){
        float nx = dx/(r+1e-6f), ny = dy/(r+1e-6f);
        float dir = (dist>=0)?1.0f:-1.0f;
        float target = rr + dir*(LAB_RING_THICK/2.0f + MZ_BALL_R + 0.5f);
        px = CX + nx*target; py = CY + ny*target;
        reflect2(nx*dir, ny*dir, ivx, ivy);
        return;
      }
    }
  }

  if (r <= GOAL_R) {
    win = true;
    gs  = GState::GAME_OVER;
    showMenu = true;
    sel = 0;
    menuCount = 2;
    Haptics::press();
  }
}

// IMU tilt (smoothed in Sensors::BNO)
static void readTilt(float &ax, float &ay){
  Sensors::BNO::read(ax, ay);
  if (Sensors::BNO::stability() >= 3){ ax *= 0.6f; ay *= 0.6f; }
}

// Menu knob rotation while menu is open (uses kptr)
static void updateMenuInput(){
  if (!kptr) return;
  int pos = kptr->get();
  int d = pos - lastKnob;
  lastKnob = pos;
  if (d != 0 && menuCount > 0){
    sel = (sel + (d>0 ? 1 : -1) + menuCount) % menuCount;
    Haptics::tap();
  }
}

void Maze::tick(float dt){
  Sensors::BNO::poll();

  if (showMenu) { updateMenuInput(); return; }
  if (!Sensors::BNO::ready() || gs != GState::RUNNING) return;

#if MAZE_TILT_DISCRETE
  // --- Discrete mode: dx,dy in {-1,0,+1} with hysteresis & debounce ---
  int dx, dy;
  Sensors::BNO::dir2D(dx, dy);

  // Map to acceleration impulses
  // Note: if your screen “right tilt” should push the ball to +x on screen, keep as is.
  // If inverted, swap signs here.
  const float ACC_STEP = 360.0f; // px/s^2 per discrete step (fast and responsive)
  vx += (-dx) * ACC_STEP * dt;   // negate X if your device axes are flipped
  vy += (-dy) * ACC_STEP * dt;

#else
  // --- Analog fallback (uses gravity tilt ~[-1..1]) ---
  float ax, ay; Sensors::BNO::read(ax, ay);
  vx += (-ax) * ACC_SCALE * dt;
  vy += (-ay) * ACC_SCALE * dt;
#endif

  // Clamp & damp
  float sp2 = vx*vx + vy*vy;
  if (sp2 > MAX_V*MAX_V){
    float s = MAX_V / sqrtf(sp2);
    vx *= s; vy *= s;
  }
  vx *= DRAG; vy *= DRAG;

  // Integrate & collide
  x += vx * dt; y += vy * dt;
  collide(x,y,vx,vy);
}


// ---- UI ----
static void drawMenuBox(TFT_eSprite &spr, const char* title,
                        const char* const* items, int count, int selIdx){
  const int w = 180, h = 110;
  const int rx = CX - w/2, ry = CY - h/2;

  spr.fillRoundRect(rx, ry, w, h, 8, Theme::BG);
  spr.drawRoundRect(rx, ry, w, h, 8, TFT_DARKGREY);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_CYAN, Theme::BG);
  spr.drawString(title, CX, ry + 16);

  spr.setTextDatum(TL_DATUM);
  for (int i=0;i<count;i++){
    int iy = ry + 36 + i*20;
    if (i == selIdx){
      spr.fillRoundRect(rx + 10, iy - 2, w - 20, 18, 4, TFT_DARKGREY);
      spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
    } else {
      spr.setTextColor(TFT_LIGHTGREY, Theme::BG);
    }
    spr.drawString(items[i], rx + 16, iy);
  }
}
static void drawPauseMenu(TFT_eSprite &spr){
  static const char* items[] = {"Resume","Replay","Main Menu"};
  drawMenuBox(spr, "PAUSED", items, 3, sel);
}
static void drawGameOverMenu(TFT_eSprite &spr){
  static const char* items[] = {"Replay","Main Menu"};
  drawMenuBox(spr, "GAME OVER", items, 2, sel);
}

void Maze::draw(TFT_eSprite &spr){
  spr.fillSprite(Theme::BG);

  uint16_t wallC = TFT_WHITE;
  for (int i=0;i<LAB_N_RINGS;i++){
    float cuts[6]; int cutN = 0;
    for (int g=0; g<gaps[i].count; ++g){
      float a0 = angNorm(gaps[i].g[g].a0), a1 = angNorm(gaps[i].g[g].a1);
      if (a0 <= a1){ cuts[cutN++]=a0; cuts[cutN++]=a1; }
      else { cuts[cutN++]=a0; cuts[cutN++]=360.0f; cuts[cutN++]=0.0f; cuts[cutN++]=a1; }
    }
    for (int a=0;a<cutN;a++) for(int b=a+1;b<cutN;b++) if (cuts[b]<cuts[a]){ float t=cuts[a]; cuts[a]=cuts[b]; cuts[b]=t; }
    float prev=0.0f; float seg[6][2]; int n=0;
    for (int c=0;c<cutN;c+=2){ float s=cuts[c], e=cuts[c+1]; if (s>prev){ seg[n][0]=prev; seg[n][1]=s; n++; } prev=e; }
    if (prev<360.0f){ seg[n][0]=prev; seg[n][1]=360.0f; n++; }
    for (int s=0;s<n;s++) Widgets::drawThickArc(spr, CX,CY, ringR[i], LAB_RING_THICK, seg[s][0], seg[s][1], wallC);
  }

  spr.drawCircle(CX,CY,GOAL_R,TFT_GREEN);
  spr.fillCircle((int)roundf(x),(int)roundf(y), MZ_BALL_R, TFT_YELLOW);

  if (showMenu){
    if (gs == GState::PAUSED)      drawPauseMenu(spr);
    else                           drawGameOverMenu(spr);
  }

  spr.pushSprite(0,0);
}

void Maze::onPress(void (*goMenu)()){
  if (showMenu){
    if (gs == GState::PAUSED){
      if (sel == 0){ showMenu = false; gs = GState::RUNNING; Haptics::tap(); }
      else if (sel == 1){ reset(); showMenu = false; Haptics::press(); }
      else { showMenu = false; Haptics::back(); goMenu(); }
    } else {
      if (sel == 0){ reset(); showMenu = false; Haptics::press(); }
      else { showMenu = false; Haptics::back(); goMenu(); }
    }
    return;
  }

  if (gs == GState::GAME_OVER){
    showMenu = true; sel = 0; menuCount = 2; return;
  }

  if (gs == GState::RUNNING){
    gs = GState::PAUSED; showMenu = true; sel = 0; menuCount = 3; Haptics::tap();
  }
}

#include "Torch.h"
#include <cstring>
#include "BNO.h"

// --- RGB565 + tiny palette torch like before ---
static inline uint16_t rgb565(uint8_t r,uint8_t g,uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static const int W = 60, H = 120;
static uint8_t  buf[W*H];
static uint16_t pal[64];
static uint32_t seed = 1;

static inline uint32_t trand(){ seed = 1664525UL*seed + 1013904223UL; return seed; }

static void buildPal(){
  for(int i=0;i<64;i++){
    float t = i / 63.0f;
    float r = 255.0f*(0.6f + 0.4f*t);
    float g = 255.0f*(0.3f + 0.7f*t);
    float b = 40.0f*t;
    if(r>255) r=255; if(g>255) g=255; if(b>255) b=255;
    pal[i] = rgb565((uint8_t)r,(uint8_t)g,(uint8_t)b);
  }
}

// --- Tilt → wind/intensity ---
// Discrete tilt from BNO085: dx,dy ∈ {-1,0,+1}
static float windLerp = 0.0f;      // smoothed wind (pixels of lateral shift per row propagation)
static const float WIND_EASE = 0.22f;   // easing factor for smoothness
static const int   WIND_MAG  = 2;       // base wind magnitude for dx=±1

// Up/down tilt modulates fuel injection at the base
// dy=+1 (tilt "up") = hotter, dy=-1 = cooler
static int baseHot0 = 48; // original: 48,40,30 for the 3 bottom rows
static int baseHot1 = 40;
static int baseHot2 = 30;

void Torch::initOnce(TFT_eSprite &){ buildPal(); }

void Torch::init(){
  std::memset(buf, 0, sizeof(buf));
  seed = millis() ^ 0x5A5A5A5A;
  windLerp = 0.0f;
}

// Convert dy to additive heat offset; returns [-HEAT_K, +HEAT_K]
static inline int heatOffsetFromDY(int dy){
  // each dy step adds/removes a few palette levels
  const int HEAT_K = 6; // tweak for stronger/weaker vertical effect
  return dy * HEAT_K;
}

void Torch::step(){
  // Poll IMU and get discrete direction
  Sensors::BNO::poll();
  int dx = 0, dy = 0;
  if (Sensors::BNO::ready()){
    Sensors::BNO::dir2D(dx, dy);
  }

  // Smooth wind target from dx; add a subtle random flutter
  float targetWind = (float)(dx * WIND_MAG);
  windLerp += (targetWind - windLerp) * WIND_EASE;

  // Small ±1 jitter so the flame feels alive even at steady tilt
  int flutter = ((trand() >> 30) & 0x3) - 1;   // -1,0,+1
  int wind = (int)lroundf(windLerp) + flutter;

  // Inject “fuel” at the bottom rows, modulated by dy
  int heatOff = heatOffsetFromDY(dy);

  for (int x = 0; x < W; x++){
    uint8_t r = (trand() >> 24) & 0xFF;
    int h0 = baseHot0 + (r >> 2) + heatOff;  // bottom
    int h1 = baseHot1 + (r >> 3) + heatOff;  // bottom+1
    int h2 = baseHot2 + (r >> 3) + heatOff;  // bottom+2
    if (h0 < 0) h0 = 0; else if (h0 > 63) h0 = 63;
    if (h1 < 0) h1 = 0; else if (h1 > 63) h1 = 63;
    if (h2 < 0) h2 = 0; else if (h2 > 63) h2 = 63;
    buf[(H-1)*W + x] = (uint8_t)h0;
    buf[(H-2)*W + x] = (uint8_t)h1;
    buf[(H-3)*W + x] = (uint8_t)h2;
  }

  // Upward diffusion with wind advection (sideways sample bias)
  for (int y = 1; y < H; y++){
    int srcY = H - y;
    int dstY = H - y - 1;

    for (int x = 0; x < W; x++){
      // take a 3-sample average from previous row with lateral wind
      int x0 = x + wind - 1;
      int x1 = x + wind;
      int x2 = x + wind + 1;
      if (x0 < 0) x0 = 0; else if (x0 >= W) x0 = W - 1;
      if (x1 < 0) x1 = 0; else if (x1 >= W) x1 = W - 1;
      if (x2 < 0) x2 = 0; else if (x2 >= W) x2 = W - 1;

      int a = buf[srcY * W + x0];
      int b = buf[srcY * W + x1];
      int c = buf[srcY * W + x2];
      int m = (a + b + c) / 3;

      // Natural cooling noise
      m -= ((trand() >> 27) & 0x7);  // 0..7
      if (m < 0) m = 0; else if (m > 63) m = 63;

      buf[dstY * W + x] = (uint8_t)m;
    }
  }
}

void Torch::render(TFT_eSprite &spr){
  spr.fillSprite(TFT_BLACK);
  const int sx = 4, sy = 2;  // scale up for screen
  for (int y = 0; y < H; y++){
    int yy = y * sy;
    uint8_t* row = &buf[y * W];
    for (int x = 0; x < W; x++){
      spr.fillRect(x * sx, yy, sx, sy, pal[row[x]]);
    }
  }
  spr.pushSprite(0, 0);
}

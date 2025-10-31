#include "measure.h"

#include "AppConfig.h"     // CX, CY, SCREEN_WIDTH/HEIGHT
#include "Theme.h"         // Theme::BG
#include "Haptics.h"
#include "Icons.h"         // FreeSansBold24pt (if not, include FreeSansBold24pt.h directly)
#include "FreeMonoOblique9pt.h"  // <-- needed for FreeMonoOblique9pt7b

namespace {
  // Config
  float g_screenMM   = 32.4f;      // active screen width in millimeters (edit if your panel differs)
  float g_pxPerMM    = (float)SCREEN_WIDTH / 32.4f;

  // State
  int   g_knobZero   = 0;          // knob baseline when entering tool
  float g_radiusMM   = 0.0f;       // current radius from center in mm

  inline int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

  void recalcScale(){
    g_pxPerMM = (float)SCREEN_WIDTH / g_screenMM;
  }

  void drawCenterCrosshair(TFT_eSprite& spr){
    const int len = 6;
    spr.drawLine(CX-len, CY,     CX+len, CY,     TFT_DARKGREY);
    spr.drawLine(CX,     CY-len, CX,     CY+len, TFT_DARKGREY);
  }

  // ---- UI draw (D at top; inches on second line with nearest 1/32") ----
void drawUI(TFT_eSprite& spr){
  spr.fillSprite(Theme::BG);

  // Optional radial guides
  const int maxRpx = (SCREEN_WIDTH/2) - 4;
  for(float rmm = 2.0f; (int)(rmm * g_pxPerMM) < maxRpx; rmm += 2.0f){
    spr.drawCircle(CX, CY, (int)roundf(rmm * g_pxPerMM), TFT_DARKGREY);
  }

  drawCenterCrosshair(spr);

  // Red ticks + line
  const int rpx   = (int)roundf(g_radiusMM * g_pxPerMM);
  const int tickH = 10;
  spr.drawFastVLine(CX - rpx, CY - tickH/2, tickH, TFT_RED);
  spr.drawFastVLine(CX + rpx, CY - tickH/2, tickH, TFT_RED);
  spr.drawLine(CX - rpx, CY, CX + rpx, CY, TFT_RED);

  // ---- Distances ----
  const float distanceMM   = 2.0f * g_radiusMM;
  const float distanceInch = distanceMM / 25.4f; // mm → in

  // Nearest 1/32"
  int whole = (int)distanceInch;
  float frac = distanceInch - whole;
  int rawNum32 = (int)roundf(frac * 32.0f);
  if (rawNum32 == 32) { rawNum32 = 0; whole++; }

  // Reduce fraction
  int num32 = rawNum32, denom = 32;
  if (num32) {
    int a = num32, b = denom;
    while (b) { int t = b; b = a % b; a = t; }
    num32 /= a; denom /= a;
  }

  // Exactness gate: only show fraction if within ±1/64"
  const float idealFrac = (num32 ? (float)num32/denom : 0.0f);
  const bool exact = fabsf(frac - idealFrac) < (1.0f/64.0f);

  // ---- Text styling ----
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);

  // Line 1: millimeters (big)
  spr.setFreeFont(&FreeSansBold24pt);
  const int yTop = 22;
  char buf1[32];
  snprintf(buf1, sizeof(buf1), "D: %.2f mm", distanceMM);
  spr.drawString(buf1, CX, yTop);

  // Line 2: inches (small; fraction only if exact)
  spr.setFreeFont(&FreeMonoOblique9pt7b);
  char buf2[48];

  const float INCH_EPS = 0.002f;             // ±0.001" tolerance
  const float step32   = 1.0f / 32.0f;       // 0.03125"

  // find nearest 1/32 multiple
  int nearestStep = (int)roundf(distanceInch / step32);
  float nearestVal = nearestStep * step32;
  float diff = fabsf(distanceInch - nearestVal);

  if (diff <= INCH_EPS) {
    // it's an exact 1/32" multiple — show nice fraction
    int whole = nearestStep / 32;
    int num   = nearestStep % 32;
    int den   = 32;

    // reduce num/den
    if (num != 0) {
      int a = num, b = den;
      while (b) { int t = b; b = a % b; a = t; }
      num /= a; den /= a;
    }

    if (num == 0) {
      snprintf(buf2, sizeof(buf2), "%.3f\" (%d\")", distanceInch, whole);
    } else if (whole > 0) {
      snprintf(buf2, sizeof(buf2), "%.3f\" (%d %d/%d\")", distanceInch, whole, num, den);
    } else {
      snprintf(buf2, sizeof(buf2), "%.3f\" (%d/%d\")", distanceInch, num, den);
    }
  } else {
    // not a clean multiple → just decimal inches
    snprintf(buf2, sizeof(buf2), "%.3f\"", distanceInch);
  }

  spr.drawString(buf2, CX, yTop + 28);
}
} // <-- CLOSE anonymous namespace

// ===== Public API =====
void Measure::init(TFT_eSprite& spr, ModulinoKnob& /*knob*/){
  recalcScale();
  spr.fillSprite(Theme::BG);
}

void Measure::enter(ModulinoKnob& knob){
  g_knobZero = knob.get();
  g_radiusMM = 0.0f;
  Haptics::press();
}

void Measure::tickAndDraw(TFT_eSprite& spr, ModulinoKnob& knob){
  const int delta = knob.get() - g_knobZero;
  g_radiusMM = 0.1f * (float)delta;

  const float maxRmm = ((SCREEN_WIDTH/2) - 6) / g_pxPerMM;
  if(g_radiusMM < 0.0f) g_radiusMM = 0.0f;
  if(g_radiusMM > maxRmm) g_radiusMM = maxRmm;

  drawUI(spr);
}

void Measure::setScreenWidthMM(float mm){
  if(mm > 1.0f) {
    g_screenMM = mm;
    recalcScale();
  }
}

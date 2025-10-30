#include "measure.h"

#include "AppConfig.h"     // CX, CY, SCREEN_WIDTH/HEIGHT
#include "Theme.h"         // Theme::BG
#include "Haptics.h"
#include "Icons.h"         // FreeSansBold24pt

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

  void drawUI(TFT_eSprite& spr){
    spr.fillSprite(Theme::BG);

    // Optional nice radial guides every ~2 mm
    const int maxRpx = (SCREEN_WIDTH/2) - 4;
    for(float rmm = 2.0f; (int)(rmm * g_pxPerMM) < maxRpx; rmm += 2.0f){
      spr.drawCircle(CX, CY, (int)roundf(rmm * g_pxPerMM), TFT_DARKGREY);
    }

    drawCenterCrosshair(spr);

    // Red ticks and guide line along X axis
    const int rpx   = (int)roundf(g_radiusMM * g_pxPerMM);
    const int tickH = 10;
    spr.drawFastVLine(CX - rpx, CY - tickH/2, tickH, TFT_RED);
    spr.drawFastVLine(CX + rpx, CY - tickH/2, tickH, TFT_RED);
    spr.drawLine(CX - rpx, CY, CX + rpx, CY, TFT_RED);

    // Bottom readout
const float distanceMM = 2.0f * g_radiusMM;

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  spr.setFreeFont(&FreeSansBold24pt);

  const int yTop = 22; // ~22px from top fits nicely on round screens
  char buf[32];
  snprintf(buf, sizeof(buf), "D: %.1f mm", distanceMM);
  spr.drawString(buf, CX, yTop);
  }
} // namespace

void Measure::init(TFT_eSprite& spr, ModulinoKnob& /*knob*/){
  recalcScale();
  // Pre-clear once so there's no flash the first frame
  spr.fillSprite(Theme::BG);
}

void Measure::enter(ModulinoKnob& knob){
  // Zero at entry so each detent = 0.1 mm from "now"
  g_knobZero = knob.get();
  g_radiusMM = 0.0f;
  Haptics::press(); // subtle feedback on entry
}

void Measure::tickAndDraw(TFT_eSprite& spr, ModulinoKnob& knob){
  // Read absolute knob and convert to 0.1 mm per detent
  const int delta = knob.get() - g_knobZero;
  g_radiusMM = 0.1f * (float)delta;

  // Clamp so ticks remain on-screen
  const float maxRmm = ((SCREEN_WIDTH/2) - 6) / g_pxPerMM;
  if(g_radiusMM < 0.0f) g_radiusMM = 0.0f;
  if(g_radiusMM > maxRmm) g_radiusMM = maxRmm;

  drawUI(spr);
}

void Measure::setScreenWidthMM(float mm){
  if(mm <= 1.0f) return;  // ignore nonsense
  g_screenMM = mm;
  recalcScale();
}

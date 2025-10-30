#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Modulino.h>

// Simple center-out measurement tool controlled by the rotary knob.
// - Each detent = 0.1 mm
// - Two symmetric red ticks from screen center
// - Bottom readout shows radius and tip-to-tip span

namespace Measure {
  // Call once at boot (after sprite init)
  void init(TFT_eSprite& spr, ModulinoKnob& knob);

  // Call when you switch into the Measure app (re-zeros knob baseline)
  void enter(ModulinoKnob& knob);

  // Call every frame while active
  void tickAndDraw(TFT_eSprite& spr, ModulinoKnob& knob);

  // Optional: set your panel's active width in millimeters (default ~32.4mm for many 1.28" 240x240)
  void setScreenWidthMM(float mm);
}

#pragma once
#include <TFT_eSPI.h>
#include <Modulino.h>

namespace Lights {
  void init(TFT_eSprite &spr, ModulinoKnob &knob);
  void tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob);

  // One-shot flag to ask the shell (GameHub.ino) to exit Lights to the main menu
  bool wantsExit();
}

#ifndef QUBE_LAUNCHER_H
#define QUBE_LAUNCHER_H

#include <TFT_eSPI.h>
#include <Modulino.h>

// Simple Launcher page that shows a 2x3 grid of 120×120 icons.
// Rotate knob to move selection; short press sends "RUN:<name>" over BLE.
// Long press triggers an exit flag you can read to go back to the main menu.
namespace Launcher {
  // Call once when entering the launcher state
  void init(TFT_eSprite &spr, ModulinoKnob &knob);

  // Call every frame while the launcher state is active
  // Renders the grid and handles knob/press input internally.
  void tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob);

  // If true, the UI is requesting to exit (long-press detected).
  // You can clear it by calling wantsExit() once (it auto-resets).
  bool wantsExit();
}

#endif // QUBE_LAUNCHER_H

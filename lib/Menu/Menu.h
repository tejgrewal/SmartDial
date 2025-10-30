#ifndef QUBE_MENU_H
#define QUBE_MENU_H

#include <TFT_eSPI.h>
#include <Modulino.h>
#include "AppConfig.h"

enum class MenuId { M_HOME, M_BULB, M_TORCH,M_MEASURE, M_PONG, M_SNAKE, M_MAZE, M_HISCORES, M_ABOUT, M_LAUNCHER };

namespace Menu {
  void init(TFT_eSprite &spr, ModulinoKnob &knob);
  void startPop();
  void tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob);
  MenuId currentId();
}

#endif // QUBE_MENU_H

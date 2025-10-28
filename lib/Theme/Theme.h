#ifndef QUBE_THEME_H
#define QUBE_THEME_H

#include <TFT_eSPI.h>

namespace Theme {
  extern uint16_t colors[10];
  extern int      themeIndex;
  extern uint16_t BG;

  inline uint16_t current(){ return colors[themeIndex]; }
}

#endif // QUBE_THEME_H
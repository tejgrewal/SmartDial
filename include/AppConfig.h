#ifndef QUBE_APPCONFIG_H
#define QUBE_APPCONFIG_H

#include <TFT_eSPI.h>

// Display
constexpr int SCREEN_WIDTH  = 240;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CX = SCREEN_WIDTH/2;
constexpr int CY = SCREEN_HEIGHT/2;

// Timing
constexpr uint32_t FRAME_US = 33000; // ~30 FPS

// App states
enum class AppState {
  STATE_MENU, STATE_PONG, STATE_SNAKE, STATE_MEASURE, STATE_HISCORES, STATE_ABOUT, STATE_MAZE, STATE_TORCH,
  STATE_LAUNCHER, STATE_LIGHTS            // <-- add this
};


#endif // QUBE_APPCONFIG_H

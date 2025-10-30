#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Modulino.h>

// Core modules
#include "Theme.h"
#include "Haptics.h"
#include "Storage.h"
#include "Menu.h"

// Games & apps
#include "Pong.h"
#include "Snake.h"
#include "Maze.h"
#include "Torch.h"
#include "Launcher.h"     // <-- NEW: Launcher (BLE GATT)
#include "Highscores.h"
#include "About.h"
#include "BNO.h"
#include "BLEHost.h"   
#include "Lights.h"    // BLE UART-like server (NimBLE)

TFT_eSPI tft;
TFT_eSprite sprite(&tft);
ModulinoKnob knob;

static AppState appState = AppState::STATE_MENU;
static uint32_t lastUs = 0;

// ---- App glue helpers ----
static void goMenu()        { appState = AppState::STATE_MENU; Menu::startPop(); }
static void startPong()     { appState = AppState::STATE_PONG; Pong::reset(); }
static void startSnake()    { appState = AppState::STATE_SNAKE; Snake::reset(); }
static void startMaze()     { appState = AppState::STATE_MAZE; Maze::reset(); }
static void startTorch()    { appState = AppState::STATE_TORCH; Torch::init(); }
static void showHighscores(){ appState = AppState::STATE_HISCORES; Storage::loadHighscores(); }
static void showAbout()     { appState = AppState::STATE_ABOUT; }
static void startLauncher() { appState = AppState::STATE_LAUNCHER; Launcher::init(sprite, knob); }
static void startLights(){ appState = AppState::STATE_LIGHTS; Lights::init(sprite, knob); }

void setup() {
  Serial.begin(115200);

  Modulino.begin();
  knob.begin();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  sprite.setSwapBytes(true);
  sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  // Init subsystems
  Haptics::begin();
  Sensors::BNO::begin();        // non-fatal if missing
  Storage::loadHighscores();

  // BLE UART-like service (desktop helper connects to this)
  BLEHost::begin("QubeLauncher");

  // Init pages/games
  Menu::init(sprite, knob);
  Pong::init(sprite, knob);
  Snake::init(sprite, knob);
  Maze::init(sprite, knob);
  Torch::initOnce(sprite);
  Highscores::init(sprite, knob);
  About::init(sprite, knob);

  lastUs = micros();
  goMenu();
}

void loop() {
// ---- Press edge (shared) ----
static bool pressed = false;
if (knob.isPressed()) {
  if (!pressed) pressed = true;
} else if (pressed) {
  pressed = false;
  Haptics::press();

  if (appState == AppState::STATE_MENU) {
    switch (Menu::currentId()) {
      case MenuId::M_HOME:      break; // reserved
      case MenuId::M_BULB:      startLights();   break;
      case MenuId::M_LAUNCHER:  startLauncher(); break;
      case MenuId::M_TORCH:     startTorch();    break;
      case MenuId::M_PONG:      startPong();     break;
      case MenuId::M_SNAKE:     startSnake();    break;
      case MenuId::M_MAZE:      startMaze();     break;
      case MenuId::M_HISCORES:  showHighscores();break;
      case MenuId::M_ABOUT:     showAbout();     break;
    }

  } else if (appState == AppState::STATE_TORCH) {
    Haptics::back();
    goMenu();

  } else if (appState == AppState::STATE_PONG) {
    Pong::onPress(goMenu);

  } else if (appState == AppState::STATE_SNAKE) {
    Snake::onPress(goMenu);

  } else if (appState == AppState::STATE_MAZE) {
    Maze::onPress(goMenu);

  } else if (appState == AppState::STATE_HISCORES) {
    Highscores::onPress(goMenu);
  
  } else if (appState == AppState::STATE_ABOUT) {
    // deliver press to About page (will call goMenu() or set its exit flag)
    About::onPress(goMenu);

  } else if (appState == AppState::STATE_LIGHTS) {
    // IMPORTANT: do nothing here — Lights handles press internally.
    // (No goMenu(); keep the app active.)

  } else if (appState == AppState::STATE_LAUNCHER) {
    // Same idea: let Launcher handle its own presses / wantsExit().
    // (No goMenu();)

  } else {
    // For any truly unknown state, you can still fall back to menu:
    // goMenu();
  }
}

  // ---- Frame pacing (~30 FPS) ----
  const uint32_t nowUs = micros();
  if (nowUs - lastUs < FRAME_US) return;
  const float dt = float(nowUs - lastUs) / 1e6f;
  lastUs = nowUs;

  // ---- State machine ----
  switch (appState) {
    case AppState::STATE_MENU:
      Menu::tickAndDraw(sprite, knob);
      break;

    case AppState::STATE_TORCH:
      Torch::step();
      Torch::render(sprite);
      break;

    case AppState::STATE_HISCORES:
      Highscores::tickAndDraw(sprite, knob);
      break;

    case AppState::STATE_ABOUT:
      About::draw(sprite);
      break;

    case AppState::STATE_PONG:
      Pong::tick(dt);
      Pong::draw(sprite);
      break;

    case AppState::STATE_SNAKE:
      Snake::tick();
      Snake::draw(sprite);
      break;

    case AppState::STATE_MAZE:
      Maze::tick(dt);
      Maze::draw(sprite);
      break;

    // ---- NEW: BLE Launcher page ----
    case AppState::STATE_LAUNCHER:
      Launcher::tickAndDraw(sprite, knob);
      if (Launcher::wantsExit()) {
        Haptics::back();
        goMenu();
      }
      break;
    case AppState::STATE_LIGHTS:
      Lights::tickAndDraw(sprite, knob);
      if (Lights::wantsExit()) { Haptics::back(); goMenu(); }
      break;
  }
}

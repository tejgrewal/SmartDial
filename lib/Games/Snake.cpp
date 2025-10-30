#include "Snake.h"
#include "AppConfig.h"
#include "Haptics.h"
#include "Theme.h"
#include "Storage.h"

struct Cell { int x; int y; };

// Grid & layout
static const int GRID = 20;
static const int CELL = 10;
static const int ORGX = CX - (GRID * CELL) / 2;
static const int ORGY = CY - (GRID * CELL) / 2 + 10;

// Snake state
static Cell snake[GRID * GRID];
static int  snakeLen = 0;
enum Dir : uint8_t { UP, RIGHTD, DOWN, LEFTD };
static Dir dir = RIGHTD;

static int  foodX = 10, foodY = 10;
static uint32_t stepMs = 180, lastStep = 0;
static bool gameOver = false;
static int  score = 0;

// Input
static ModulinoKnob* k = nullptr;
static int lastKnob = 0, accum = 0;
static const int TURN_THRESH = 2; // encoder ticks needed to turn
static bool turnUsed = false;

// Pause / menu
enum class GState { RUNNING, PAUSED, GAME_OVER };
static GState gs = GState::RUNNING;

static bool showMenu = false;
static int  sel = 0;       // selection index
static int  menuCount = 0; // items in current menu (3 for pause, 2 for game over)

// RNG (LCG)
static uint32_t lcg = 1234567;
static uint32_t frand(){ lcg = 1664525UL * lcg + 1013904223UL; return lcg; }

// ------------------------------ Radial food placement ------------------------------
// Build a set of unique board cells sampled along multiple angles and radii, so apples
// appear on a "radial grid" centered at the playfield center.
struct I2 { int x; int y; };
static I2 radialCells[GRID * GRID];
static int radialCount = 0;

static bool containsCell(const I2* arr, int n, int x, int y){
  for (int i = 0; i < n; ++i) if (arr[i].x == x && arr[i].y == y) return true;
  return false;
}

static void buildRadialCells(){
  radialCount = 0;

  // center in grid coordinates (not pixels)
  const float cx = (GRID - 1) * 0.5f;
  const float cy = (GRID - 1) * 0.5f;

  // choose a decent spoke/radius set
  const int spokes = 24;               // every 15 degrees
  const int rMin   = 2;
  const int rMax   = (GRID / 2) - 1;   // keep off the very edge

  for (int s = 0; s < spokes; ++s){
    float ang = (s * 360.0f / spokes) * 0.01745329252f; // deg->rad
    float ux = cosf(ang), uy = sinf(ang);

    for (int r = rMin; r <= rMax; ++r){
      int gx = (int)roundf(cx + ux * r);
      int gy = (int)roundf(cy + uy * r);
      if (gx < 0 || gx >= GRID || gy < 0 || gy >= GRID) continue;
      if (!containsCell(radialCells, radialCount, gx, gy)){
        radialCells[radialCount++] = { gx, gy };
      }
    }
  }

  // ensure we still have some candidates; if not, fall back later
}

static bool snakeOccupies(int x, int y){
  for (int i = 0; i < snakeLen; ++i)
    if (snake[i].x == x && snake[i].y == y) return true;
  return false;
}

static void placeFoodRandom(){
  // classic fallback: uniform random over cells that are not snake
  while (true){
    foodX = frand() % GRID;
    foodY = frand() % GRID;
    if (!snakeOccupies(foodX, foodY)) break;
  }
}

static void placeFoodRadial(){
  if (radialCount == 0) { buildRadialCells(); }
  if (radialCount == 0) { placeFoodRandom(); return; }

  // try random index into the radial list, walk until a free slot is found
  int start = frand() % radialCount;
  for (int i = 0; i < radialCount; ++i){
    int idx = (start + i) % radialCount;
    int x = radialCells[idx].x;
    int y = radialCells[idx].y;
    if (!snakeOccupies(x, y)){
      foodX = x; foodY = y;
      return;
    }
  }
  // If somehow all are blocked (very long snake), fall back
  placeFoodRandom();
}

// ------------------------------ Game setup ------------------------------
void Snake::init(TFT_eSprite &, ModulinoKnob &knob){
  k = &knob;
  lastKnob = knob.get();
  buildRadialCells();
}

void Snake::reset(){
  snakeLen = 3;
  snake[0] = { GRID/2 - 1, GRID/2 };
  snake[1] = { GRID/2    , GRID/2 };
  snake[2] = { GRID/2 + 1, GRID/2 };
  dir = RIGHTD;

  score = 0;
  stepMs = 180;
  lastStep = millis();
  gameOver = false;
  gs = GState::RUNNING;

  accum = 0;
  turnUsed = false;

  placeFoodRadial();

  // menu reset
  showMenu = false;
  sel = 0;
  menuCount = 0;
}

// ------------------------------ Turning ------------------------------
static void turnLeft(){
  Dir nd = (dir == UP) ? LEFTD : (dir == LEFTD) ? DOWN : (dir == DOWN) ? RIGHTD : UP;
  if (!((dir == UP && nd == DOWN) || (dir == DOWN && nd == UP) ||
        (dir == LEFTD && nd == RIGHTD) || (dir == RIGHTD && nd == LEFTD))) {
    dir = nd;
  }
}
static void turnRight(){
  Dir nd = (dir == UP) ? RIGHTD : (dir == RIGHTD) ? DOWN : (dir == DOWN) ? LEFTD : UP;
  if (!((dir == UP && nd == DOWN) || (dir == DOWN && nd == UP) ||
        (dir == LEFTD && nd == RIGHTD) || (dir == RIGHTD && nd == LEFTD))) {
    dir = nd;
  }
}

// ------------------------------ Input update ------------------------------
static void updateInput(){
  int pos = k->get();
  int d = pos - lastKnob;
  lastKnob = pos;

  if (showMenu){
    // rotate selection in menus
    if (d != 0 && menuCount > 0){
      sel = (sel + (d > 0 ? 1 : -1) + menuCount) % menuCount;
      Haptics::tap();
    }
    return;
  }

  // game turning (coalesce ticks to avoid jitter)
  if (d != 0 && !turnUsed){
    accum += (d > 0) ? 1 : -1;
    if (accum >= TURN_THRESH){
      turnRight(); turnUsed = true; accum = 0; Haptics::tap();
    } else if (accum <= -TURN_THRESH){
      turnLeft();  turnUsed = true; accum = 0; Haptics::tap();
    }
  }
}

// ------------------------------ Tick ------------------------------
void Snake::tick(){
  updateInput();

  if (gs == GState::PAUSED || showMenu) return;

  if (gameOver) { gs = GState::GAME_OVER; return; }

  uint32_t now = millis();
  if (now - lastStep < stepMs) return;
  lastStep = now;
  turnUsed = false;

  // advance head
  Cell head = snake[snakeLen - 1];
  if (dir == UP)      head.y--;
  else if (dir == DOWN)  head.y++;
  else if (dir == LEFTD) head.x--;
  else                   head.x++;

  // wrap
  if (head.x < 0) head.x = GRID - 1; else if (head.x >= GRID) head.x = 0;
  if (head.y < 0) head.y = GRID - 1; else if (head.y >= GRID) head.y = 0;

  // self-collision
  for (int i = 0; i < snakeLen; ++i){
    if (snake[i].x == head.x && snake[i].y == head.y){
      gameOver = true;
      Storage::saveSnake(score);
      return;
    }
  }

  // eat or move
  if (head.x == foodX && head.y == foodY){
    snake[snakeLen++] = head;
    score += 10;
    if (stepMs > 80) stepMs -= 5;
    placeFoodRadial();
  } else {
    for (int i = 0; i < snakeLen - 1; ++i) snake[i] = snake[i + 1];
    snake[snakeLen - 1] = head;
  }
}

// ------------------------------ Drawing ------------------------------
static void drawMenuBox(TFT_eSprite &spr, const char* title,
                        const char* const* items, int count, int selIdx){
  // modal panel
  const int w = 180, h = 110;
  const int x = CX - w/2, y = CY - h/2;
  // simple dim: draw a subtle frame (leave background as-is to keep perf)
  spr.drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
  spr.fillRoundRect(x, y, w, h, 8, Theme::BG);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_CYAN, Theme::BG);
  spr.drawString(title, CX, y + 16);

  spr.setTextDatum(TL_DATUM);
  for (int i = 0; i < count; ++i){
    int iy = y + 36 + i * 20;
    if (i == selIdx){
      spr.fillRoundRect(x + 10, iy - 2, w - 20, 18, 4, TFT_DARKGREY);
      spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
    } else {
      spr.setTextColor(TFT_LIGHTGREY, Theme::BG);
    }
    spr.drawString(items[i], x + 16, iy);
  }
}

static void drawPauseMenu(TFT_eSprite &spr){
  static const char* items[] = { "Resume", "Replay", "Main Menu" };
  drawMenuBox(spr, "PAUSED", items, 3, sel);
}

static void drawGameOverMenu(TFT_eSprite &spr){
  static const char* items[] = { "Replay", "Main Menu" };
  drawMenuBox(spr, "GAME OVER", items, 2, sel);
}

void Snake::draw(TFT_eSprite &spr){
  spr.fillSprite(Theme::BG);

  // Draw snake first…
  for (int i = 0; i < snakeLen; ++i){
    int x = ORGX + snake[i].x * CELL;
    int y = ORGY + snake[i].y * CELL;
    spr.fillRoundRect(x + 1, y + 1, CELL - 2, CELL - 2, 3, TFT_GREEN);
  }

  // …then apple on top so it never "hides"
  int fx = ORGX + foodX * CELL + CELL / 2;
  int fy = ORGY + foodY * CELL + CELL / 2;
  spr.fillCircle(fx, fy, CELL/2 - 1, TFT_RED);

  // HUD
  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(TFT_WHITE, Theme::BG);
  char b[24]; snprintf(b, sizeof(b), "SCORE %d", score);
  spr.drawString(b, CX, SCREEN_HEIGHT - 6);

  // Overlays
  if (gs == GState::GAME_OVER){
    // legacy "game over" text (kept) under menu
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_YELLOW, Theme::BG);
    spr.drawString("GAME OVER", CX, CY - 10);
    spr.setTextColor(TFT_WHITE, Theme::BG);
    char s[24]; snprintf(s, sizeof(s), "SCORE %d", score);
    spr.drawString(s, CX, CY + 14);
  }

  if (showMenu){
    if (gs == GState::PAUSED)      drawPauseMenu(spr);
    else /* GAME_OVER */            drawGameOverMenu(spr);
  }

  spr.pushSprite(0, 0);
}

// ------------------------------ Button / menu logic ------------------------------
void Snake::onPress(void (*goMenu)()){
  if (showMenu){
    // act on selection
    if (gs == GState::PAUSED){
      if (sel == 0){        // Resume
        showMenu = false;
        gs = GState::RUNNING;
        Haptics::tap();
      } else if (sel == 1){ // Replay
        reset();
        showMenu = false;
        Haptics::press();
      } else {              // Main Menu
        showMenu = false;
        Haptics::back();
        goMenu();
      }
    } else { // GAME_OVER menu (Replay / Main)
      if (sel == 0){
        reset();
        showMenu = false;
        Haptics::press();
      } else {
        showMenu = false;
        Haptics::back();
        goMenu();
      }
    }
    return;
  }

  // If game over, open GO menu; otherwise open pause menu
  if (gameOver || gs == GState::GAME_OVER){
    gs = GState::GAME_OVER;
    showMenu = true;
    sel = 0;
    menuCount = 2;
    return;
  }

  // Toggle pause menu
  if (gs == GState::RUNNING){
    gs = GState::PAUSED;
    showMenu = true;
    sel = 0;
    menuCount = 3;
    Haptics::tap();
  } else if (gs == GState::PAUSED){
    // Quick resume on press if you prefer:
    // gs = GState::RUNNING; showMenu = false;
    // But we keep menu until selection.
  }
}
#pragma once
#include <TFT_eSPI.h>
#include <Arduino.h>


static inline void drawIconOpaque(TFT_eSprite &spr, int x, int y,
const uint16_t *progmem565,
uint16_t w, uint16_t h) {
static uint16_t line[240];
for (uint16_t row = 0; row < h; ++row) {
for (uint16_t col = 0; col < w; ++col)
line[col] = pgm_read_word(&progmem565[row * w + col]);
spr.pushImage(x, y + row, w, 1, line);
}
}
static inline void drawIconTransparent(TFT_eSprite &spr, int x, int y,
const uint16_t *progmem565,
uint16_t w, uint16_t h,
uint16_t colorkey) {
for (uint16_t row = 0; row < h; ++row) {
for (uint16_t col = 0; col < w; ++col) {
uint16_t c = pgm_read_word(&progmem565[row * w + col]);
if (c != colorkey) spr.drawPixel(x + col, y + row, c);
}
}
}
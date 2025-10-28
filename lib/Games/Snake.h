#pragma once
#include <TFT_eSPI.h>
#include <Modulino.h>
namespace Snake{
void init(TFT_eSprite &, ModulinoKnob &);
void reset();
void tick();
void draw(TFT_eSprite &spr);
void onPress(void (*goMenu)());
}
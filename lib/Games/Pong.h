#pragma once
#include <TFT_eSPI.h>
#include <Modulino.h>
namespace Pong{
void init(TFT_eSprite &, ModulinoKnob &);
void reset();
void tick(float dt);
void draw(TFT_eSprite &spr);
void onPress(void (*goMenu)());
}
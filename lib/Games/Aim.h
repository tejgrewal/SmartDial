#pragma once
#include <TFT_eSPI.h>
#include <Modulino.h>

namespace Aim{
void init(TFT_eSprite &spr, ModulinoKnob &knob);
void reset();
void tick(float dt);
void draw(TFT_eSprite &spr);
void onPress(void (*goMenu)());
} // namespace Aim

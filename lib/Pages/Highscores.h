#pragma once
#include <TFT_eSPI.h>
#include <Modulino.h>
namespace Highscores{
void init(TFT_eSprite &spr, ModulinoKnob &knob);
void tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob);
void onPress(void (*goMenu)());
}
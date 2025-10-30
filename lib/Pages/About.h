#ifndef PAGES_ABOUT_H
#define PAGES_ABOUT_H

#include <TFT_eSPI.h>
#include <Modulino.h>

class About {
public:
  static void init(TFT_eSprite &spr, ModulinoKnob &knob);
  static void tickAndDraw(TFT_eSprite &spr, ModulinoKnob &knob); // carousel-style draw (handles knob)
  static void draw(TFT_eSprite &spr);                           // optional direct draw
  static void onPress(void (*goMenu)());                        // handle knob press (back)
};

#endif
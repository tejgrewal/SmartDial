#pragma once
#include <TFT_eSPI.h>
namespace Torch{ void initOnce(TFT_eSprite &); void init(); void step(); void render(TFT_eSprite &spr); }
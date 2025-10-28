#pragma once
#include <Adafruit_BNO08x.h>
namespace Sensors{ namespace BNO{ bool begin(); bool ready(); void read(float &ax, float &ay); } }
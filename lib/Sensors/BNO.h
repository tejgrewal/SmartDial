#pragma once
#include <Adafruit_BNO08x.h>

namespace Sensors {
namespace BNO {

// Init / lifecycle
bool begin();
bool ready();
void poll();

// Simple gravity-based tilt read (no heavy fusion)
void read(float &ax, float &ay);  // ~[-1..1] using GRAVITY normalization

// NEW: discrete 2D direction with hysteresis and debounce
// dx,dy ∈ {-1,0,+1} where +x is right tilt, +y is up tilt (adjust in Maze if you want)
void dir2D(int &dx, int &dy);

// Raw snapshots
void lastAccel(float &ax, float &ay, float &az);
void lastGyro (float &gx, float &gy, float &gz);
void lastGrav (float &gx, float &gy, float &gz);

// Stability classifier 0..3-ish
uint8_t stability();

// Event detectors
bool     popTap();
bool     popShake();
bool     popStepDetected();
bool     popSignificantMotion();
uint32_t stepCount();

} // namespace BNO
} // namespace Sensors

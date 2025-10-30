#include "BNO.h"

static Adafruit_BNO08x bno;
static sh2_SensorValue_t evt;
static bool ok = false;

// latest raw
static volatile float _ax=0,_ay=0,_az=0;
static volatile float _gx=0,_gy=0,_gz=0;
static volatile float _gravx=0,_gravy=0,_gravz=0;

// simple tilt (gravity only)
static float tiltX = 0.0f, tiltY = 0.0f;
static uint32_t lastMicros = 0;

// classifiers / detectors
static volatile uint8_t  _stability = 0;
static volatile uint32_t _stepCount = 0;
static volatile bool _tap = false, _shake = false, _step = false, _significant = false;

// discrete direction state (hysteresis + debounce)
static int  dX = 0, dY = 0;                 // -1,0,+1
static const float TH_HI = 0.22f;           // enter tilted
static const float TH_LO = 0.12f;           // exit back to neutral (hysteresis)
static uint32_t lastFlipMsX = 0, lastFlipMsY = 0;
static const uint32_t FLIP_DEBOUNCE_MS = 60;

static inline float clampf(float v, float lo, float hi){
  return (v<lo)?lo : (v>hi)?hi : v;
}

bool Sensors::BNO::begin(){
  if (!bno.begin_I2C()) return false;

  // Keep it light: gravity @ 50–100 Hz, plus (optionally) accel/gyro if you need them
  bno.enableReport(SH2_GRAVITY,              10000);  // 100 Hz for smoothness, adjust to 20000 for 50 Hz
  bno.enableReport(SH2_ACCELEROMETER,        20000);  // 50 Hz (optional fallback)
  bno.enableReport(SH2_GYROSCOPE_CALIBRATED, 20000);  // 50 Hz (not used by default)

  // Nice-to-have simple classifiers (cheap)
  bno.enableReport(SH2_STABILITY_CLASSIFIER, 20000);  // 50 Hz
  bno.enableReport(SH2_TAP_DETECTOR,         0);
  bno.enableReport(SH2_SHAKE_DETECTOR,       0);
  bno.enableReport(SH2_SIGNIFICANT_MOTION,   0);
  bno.enableReport(SH2_STEP_DETECTOR,        0);
  bno.enableReport(SH2_STEP_COUNTER,         200000); // 5 Hz

  ok = true;
  lastMicros = micros();
  return ok;
}

bool Sensors::BNO::ready(){ return ok; }

void Sensors::BNO::poll(){
  if (!ok) return;
  while (bno.getSensorEvent(&evt)){
    switch (evt.sensorId){
      case SH2_ACCELEROMETER:
        _ax = evt.un.accelerometer.x;
        _ay = evt.un.accelerometer.y;
        _az = evt.un.accelerometer.z;
        break;
      case SH2_GYROSCOPE_CALIBRATED:
        _gx = evt.un.gyroscope.x;
        _gy = evt.un.gyroscope.y;
        _gz = evt.un.gyroscope.z;
        break;
      case SH2_GRAVITY:
        _gravx = evt.un.gravity.x;
        _gravy = evt.un.gravity.y;
        _gravz = evt.un.gravity.z;
        break;
      case SH2_STABILITY_CLASSIFIER:
        _stability = evt.un.stabilityClassifier.classification;
        break;
      case SH2_TAP_DETECTOR:       _tap = true; break;
      case SH2_SHAKE_DETECTOR:     _shake = true; break;
      case SH2_SIGNIFICANT_MOTION: _significant = true; break;
      case SH2_STEP_DETECTOR:      _step = true; break;
      case SH2_STEP_COUNTER:       _stepCount = evt.un.stepCounter.steps; break;
      default: break;
    }
  }

  // normalize gravity to ~g
  const float g = 9.80665f;
  float gx = _gravx / g;
  float gy = _gravy / g;

  // startup fallback: accel when gravity not yet valid
  if (gx == 0 && gy == 0){
    gx = _ax / g;
    gy = _ay / g;
  }

  // light low-pass for stability (no gyro fusion)
  const float aLPF = 0.20f;
  tiltX = (1.0f - aLPF) * tiltX + aLPF * gx;
  tiltY = (1.0f - aLPF) * tiltY + aLPF * gy;

  tiltX = clampf(tiltX, -1.5f, 1.5f);
  tiltY = clampf(tiltY, -1.5f, 1.5f);

  // update discrete directions with hysteresis & debounce
  uint32_t nowMs = millis();

  // X axis
  int targetX = dX;
  if      (tiltX >  TH_HI) targetX = +1;
  else if (tiltX < -TH_HI) targetX = -1;
  else if (fabsf(tiltX) < TH_LO) targetX = 0;

  if (targetX != dX && (nowMs - lastFlipMsX) >= FLIP_DEBOUNCE_MS){
    dX = targetX; lastFlipMsX = nowMs;
  }

  // Y axis
  int targetY = dY;
  if      (tiltY >  TH_HI) targetY = +1;
  else if (tiltY < -TH_HI) targetY = -1;
  else if (fabsf(tiltY) < TH_LO) targetY = 0;

  if (targetY != dY && (nowMs - lastFlipMsY) >= FLIP_DEBOUNCE_MS){
    dY = targetY; lastFlipMsY = nowMs;
  }
}

void Sensors::BNO::read(float &ax, float &ay){
  poll();
  ax = tiltX; ay = tiltY;
}

void Sensors::BNO::dir2D(int &dx, int &dy){
  poll();
  dx = dX; dy = dY;
}

void Sensors::BNO::lastAccel(float &ax,float &ay,float &az){ ax=_ax; ay=_ay; az=_az; }
void Sensors::BNO::lastGyro (float &gx,float &gy,float &gz){ gx=_gx; gy=_gy; gz=_gz; }
void Sensors::BNO::lastGrav (float &gx,float &gy,float &gz){ gx=_gravx; gy=_gravy; gz=_gravz; }

uint8_t  Sensors::BNO::stability(){ return _stability; }
bool Sensors::BNO::popTap(){ bool v=_tap; _tap=false; return v; }
bool Sensors::BNO::popShake(){ bool v=_shake; _shake=false; return v; }
bool Sensors::BNO::popStepDetected(){ bool v=_step; _step=false; return v; }
bool Sensors::BNO::popSignificantMotion(){ bool v=_significant; _significant=false; return v; }
uint32_t Sensors::BNO::stepCount(){ return _stepCount; }

#include "BNO.h"
static Adafruit_BNO08x bno; static sh2_SensorValue_t evt; static bool ok=false;
bool Sensors::BNO::begin(){ if(bno.begin_I2C()){ bno.enableReport(SH2_ACCELEROMETER,5000); ok=true; } return ok; }
bool Sensors::BNO::ready(){ return ok; }
void Sensors::BNO::read(float &ax,float &ay){ ax=ay=0; if(!ok) return; while(bno.getSensorEvent(&evt)){ if(evt.sensorId==SH2_ACCELEROMETER){ ax=evt.un.accelerometer.x; ay=evt.un.accelerometer.y; } } }
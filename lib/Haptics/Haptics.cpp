#include "Haptics.h"
#include <Wire.h>
static Adafruit_DRV2605 drv; static bool ready=false; static uint32_t lastTapMs=0;
bool Haptics::begin(){ if(drv.begin()){ drv.selectLibrary(1); drv.setMode(DRV2605_MODE_INTTRIG); ready=true; } return ready; }
static inline void play(uint8_t e){ if(!ready) return; drv.setWaveform(0,e); drv.setWaveform(1,0); drv.go(); }
void Haptics::tap(){ uint32_t m=millis(); if(m-lastTapMs<30) return; lastTapMs=m; play(2); }
void Haptics::press(){ play(1); }
void Haptics::back(){ play(4); }
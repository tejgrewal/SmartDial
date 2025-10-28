#ifndef QUBE_BLEHOST_H
#define QUBE_BLEHOST_H
#include <Arduino.h>

namespace BLEHost {
  void begin(const char* deviceName = "SMDIAL");  // start GATT server
  bool ready();                                          // is a client connected?
  bool send(const char* msg);                            // (optional) notify to PC
  void onConnect(void (*cb)() = nullptr);
  void onDisconnect(void (*cb)() = nullptr);
}
#endif

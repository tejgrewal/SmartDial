#include "BLEHost.h"
#include <NimBLEDevice.h>

// Simple UART-like service (Nordic UART Service-compatible UUIDs)
static NimBLEServer*       g_server = nullptr;
static NimBLECharacteristic *g_tx   = nullptr; // notify to PC
static NimBLECharacteristic *g_rx   = nullptr; // writes from PC
static volatile bool        g_connected = false;

static void (*g_onConn)() = nullptr;
static void (*g_onDisc)() = nullptr;

// NUS UUIDs
static const NimBLEUUID UUID_SVC("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static const NimBLEUUID UUID_RX ("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // Write
static const NimBLEUUID UUID_TX ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // Notify

struct ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo&){ g_connected = true; if(g_onConn) g_onConn(); }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo&){ g_connected = false; if(g_onDisc) g_onDisc(); s->startAdvertising(); }
};

struct RxCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) {
    // Optional: handle PC->device messages if you want
    std::string v = c->getValue();
    Serial.printf("[BLE RX] %s\n", v.c_str());
  }
};

namespace BLEHost {

void begin(const char* deviceName){
  NimBLEDevice::init(deviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // max
  g_server = NimBLEDevice::createServer();
  g_server->setCallbacks(new ServerCB());

  NimBLEService* svc = g_server->createService(UUID_SVC);

  g_tx = svc->createCharacteristic(UUID_TX, NIMBLE_PROPERTY::NOTIFY);
  g_rx = svc->createCharacteristic(UUID_RX, NIMBLE_PROPERTY::WRITE);
  g_rx->setCallbacks(new RxCB());

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_SVC);
  adv->addServiceUUID(UUID_SVC);
  adv->start();
  Serial.println("[BLE] Advertising as QubeLauncher (UART service)");
}

bool ready(){ return g_connected; }

bool send(const char* msg){
  if(!g_connected || !g_tx) return false;
  g_tx->setValue((uint8_t*)msg, strlen(msg));
  g_tx->notify();
  return true;
}

void onConnect(void (*cb)()){ g_onConn = cb; }
void onDisconnect(void (*cb)()){ g_onDisc = cb; }

} // namespace

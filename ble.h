#pragma once
#include <Arduino.h>

#define BLE_SLAVE_UUID "9112C615-F25C-41D1-8292-AA39BD2E3387"

bool initBLE(void);

bool isBLEConnected(); 

void BLESend(const String& msg);
bool BLEReceive(String &cmd);

// For model weight exchange
bool BLESendModel(float* weights, size_t len);
bool BLEReceiveModel(float* weights, size_t len);
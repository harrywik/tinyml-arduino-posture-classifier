#pragma once
#include <Arduino.h>

#define BLE_SLAVE_UUID "9112C615-F25C-41D1-8292-AA39BD2E3387"
#define BLE_CHARACTERISTIC_UUID "4ADFA629-DC24-4C30-B65A-4950FB6A581E"
#define MAX_CHUNK_LENGTH 48

// Protocol message types
enum BLEMsgType {
  MSG_TYPE_WEIGHTS = 0,     // Weight data
  MSG_TYPE_BATCH_COUNT = 1, // Batch count
  MSG_TYPE_DONE = 2         // Exchange completion signal [UNUSED]
};

bool initBLE(void);

bool isBLEConnected(void); 
void readvertiseBLE(void);

void BLESend(const String& msg);
bool BLEReceive(String &cmd);

// For model weight exchange
bool BLESendModel(float* weights, size_t len);
bool BLEReceiveModel(float* weights, size_t len);

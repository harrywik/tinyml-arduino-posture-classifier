#pragma once
#include <Arduino.h>
#include "io.h"

#define BLE_SLAVE_UUID "bc58da3e-2db6-4e56-942a-699d8594e8a3"
#define BLE_CHARACTERISTIC_UUID "ab0addfd-cf41-46a7-8190-beb6e07de642"
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

void deinitAsCentral(void);
void deinitAsPeripheral(void);
bool startCentralService(uuid peripheralUUID);
bool attemptConnectionToPeripheral(uuid peripheralUUID);
bool weightShareSend(WeightShareBLEMode mode, BLEMsgType type, uint8_t* data, size_t bytes);
bool weightShareReceive(WeightShareBLEMode mode, BLEMsgType type, uint8_t* data, size_t bytes);

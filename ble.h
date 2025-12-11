#pragma once
#include <Arduino.h>
#include "io.h"

#define BLE_SLAVE_UUID "97BC2A91-292A-4B46-B4A0-6265F274F98D"
#define BLE_CHARACTERISTIC_UUID "C5647BAA-3C6D-4FBD-9863-3D8F71FA60EF"
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

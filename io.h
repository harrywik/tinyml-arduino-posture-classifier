#pragma once
#include "button.h"
#include <Arduino.h>

enum WeightShareBLEMode { WS_BLE_CENTRAL, WS_BLE_PERIPHERAL };

enum SerialCommandType {
  CMD_NONE,
  CMD_TRAIN,
  CMD_VAL,
  CMD_VAL_DONE,
  CMD_INFER,
  CMD_RESET,
  CMD_PERSIST,
  CMD_SHARE_WEIGHTS
};

typedef char uuid[37];

enum IOBackend { IO_SERIAL, IO_BLE };

class IO {
public:
  // auto-detect available backend within timeout
  void setBackend(CommunicationMode mode);
  // send string message
  void send(const String &msg);
  // receive command
  SerialCommandType receive();
  // request label input
  bool getLabel(uint8_t *labelBuffer, uint16_t nSamples);
  // receive counterparty UUID address
  bool getUUID(void);
  // model weight exchange, for federated learning
  bool sendModel(float *weights, size_t len);
  bool sendNBatches(const uint16_t n_a, size_t len);
  bool receiveModel(float *weights, size_t len);
  bool receiveNBatches(uint16_t *n_b, size_t len);

private:
  IOBackend currentBackend = IO_SERIAL;
  WeightShareBLEMode currentBLEMode = WS_BLE_PERIPHERAL;
  uuid peripheral = {};
};

extern IO Coms;

#pragma once
#include <Arduino.h> 
#include "button.h"

enum SerialCommandType {
    CMD_NONE,
    CMD_TRAIN,
    //
    // Uncomment when eval branch works
    //
    // CMD_VAL,
    // CMD_VAL_DONE,
    CMD_INFER,
    CMD_RESET,
    CMD_PERSIST,
    CMD_SHARE_WEIGHTS
};

enum IOBackend {
    IO_SERIAL,
    IO_BLE
};

class IO {
public:
    // auto-detect available backend within timeout
    void setBackend(CommunicationMode mode); 
    // send string message
    void send(const String& msg);
    // receive command
    SerialCommandType receive();
    // request label input
    bool getLabel(uint8_t* labelBuffer, uint16_t nSamples);
    // model weight exchange, for federated learning
    bool sendModel(float* weights, size_t len);
    bool receiveModel(float* weights, size_t len);
    
private:
    IOBackend currentBackend = IO_SERIAL;
};

extern IO Coms;

#pragma once
#include <Arduino.h> 
#include <stddef.h> // for size_t
#include "button.h"

enum SerialCommandType {
    CMD_NONE,
    CMD_COLLECT,
    CMD_TRAIN,
    CMD_VAL,
    CMD_INFER,
    CMD_STOP,
    CMD_SHARE_WEIGHTS
};

enum IOBackend {
    IO_SERIAL,
    IO_BLE
};

class IO {
public:
    // initialize
    void begin(); 
    
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
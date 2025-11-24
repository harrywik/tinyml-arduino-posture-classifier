#pragma once
#include <Arduino.h>

// Maximum length of a command line
#define MAX_LINE_LENGTH 64

// Command types
enum SerialCommandType {
    CMD_NONE,
    CMD_LABEL,
    CMD_TRAIN,
    CMD_INFER,
    CMD_STOP
};

// Parsed command structure
struct SerialCommand {
    SerialCommandType type;
    uint8_t label;  // 0=SITTING, 1=STANDING, 2=LYING, 3=ACTIVE
};

// Read label
uint8_t parseLabel(const String& str);

// Initialize serial communication
void initSerial();

// Check for a new command, returns CMD_NONE if none available
SerialCommand readSerialCommand();

// Send a training loss update
void sendTrainLoss(float loss);

// Send a validation loss update
void sendValLoss(float loss);

// Send prediction result
void sendPrediction(uint8_t label);

// Send acknowledgement or generic message
void sendMessage(const String& msg);


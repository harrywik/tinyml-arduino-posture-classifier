#pragma once
#include <Arduino.h>

// Maximum length of a command line
#define MAX_LINE_LENGTH 64

// Command types
enum SerialCommandType {
    CMD_NONE,
    CMD_COLLECT,
    CMD_TRAIN,
    CMD_VAL,
    CMD_INFER,
    CMD_STOP
};

// Initialize serial communication
void initSerial();

// Check for a new command, returns CMD_NONE if none available
SerialCommandType readSerialCommand();

// Send acknowledgement or generic message
void sendMessage(const String& msg);

// Get label from user
bool getLabel(uint8_t (&labelBuffer)[WINDOW_SIZE], uint16_t nSamples);

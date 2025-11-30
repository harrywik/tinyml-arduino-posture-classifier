#pragma once
#include <Arduino.h>
#include "io.h" 

// Maximum length of a command line
#define MAX_LINE_LENGTH 64

// Command types
// enum SerialCommandType {
//     CMD_NONE,
//     CMD_COLLECT,
//     CMD_TRAIN,
//     CMD_VAL,
//     CMD_INFER,
//     CMD_STOP
// }; /// moved to io.h

// Initialize serial communication
void initSerial();

bool readRawSerialLine(String& out);
SerialCommandType parseSerialCommand(String& line);

// Check for a new command, returns CMD_NONE if none available
// SerialCommandType readSerialCommand(); /// split into readRawSerialLine and parseSerialCommand

///  move to io.h
// Send acknowledgement or generic message
// void sendMessage(const String& msg);

// // Get label from user
// bool getLabel(uint8_t (&labelBuffer)[WINDOW_SIZE], uint16_t nSamples);

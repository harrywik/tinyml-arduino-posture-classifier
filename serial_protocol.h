#pragma once
#include <Arduino.h>
#include "io.h" 

// Maximum length of a command line
#define MAX_LINE_LENGTH 64

// Initialize serial communication
void initSerial();

bool readRawSerialLine(String& out);
SerialCommandType parseSerialCommand(String& line);


#pragma once
#include <Arduino.h>

// Maximum length of a command line
#define MAX_LINE_LENGTH 64

// Maximum lengths for data streams of stored features + label
#define MAX_FLOAT_LEN 12 
// Max length for uint8_t (255\0) -> 3 characters.
#define MAX_UINT8_LEN 3
// Number of delimiters (commas) is equal to NUM_FEATURES.
#define NUM_DELIMITERS NUM_FEATURES 
// Null terminator
#define NULL_TERM_LEN 1

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

// Set all windows with their features and labels
void parseDataStream(FeatureVector (&windowBuffer)[WINDOW_SIZE], uint16_t* nSamples, uint8_t (&labelsBuffer)[WINDOW_SIZE]);

// Parse a single Window
bool parseWindow(FeatureVector& featureWindow, uint8_t* label, char* dataBuffer, size_t bufferSize);

// Send features to external storage
void transmitCollectedWindow(FeatureVector (&window)[WINDOW_SIZE]);

// Send prediction result
void sendPrediction(uint8_t label, OperationMode mode);

// Send acknowledgement or generic message
void sendMessage(const String& msg);


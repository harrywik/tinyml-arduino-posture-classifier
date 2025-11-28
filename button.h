#pragma once
#define LARGE_BUTTON 13

enum CommunicationMode {
	USB,
	BLE
};

typedef void (*CallbackFunction)(void);

void initButton(void);

void onButtonPress(CallbackFunction cb);

CommunicationMode getCommunicationMode(void);

bool buttonPressIgnore(void);


// TODO: 
// maybe have LED separate?

void communicateBLEMode(void);

void communicateUSBMode(void);

void communicatePersistance(void);

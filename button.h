#pragma once
#define LARGE_BUTTON 13

enum CommunicationMode {
	USB,
	BLE
};

void initButton(void);

CommunicationMode getCommunicationMode(void);

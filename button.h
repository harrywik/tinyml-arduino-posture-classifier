#pragma once

enum CommunicationMode {
	USB,
	BLE
};
	
void initButton(void);

CommunicationMode getCommunicationMode(void);

void communicateBLEMode(void);

void communicateUSBMode(void);

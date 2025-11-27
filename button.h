#pragma once

enum CommunicationMode {
	USB,
	BLE,
	NOT_INITIATED
};
	
void initButton(void);

CommunicationMode getCommunicationMode(void);


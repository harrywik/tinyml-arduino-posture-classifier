#pragma once

enum CommunicationMode {
	USB,
	BLE,
	NOT_INITIATED
}

CommunicationMode getCommunicationMode(void);


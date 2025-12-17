#pragma once
#define LARGE_BUTTON 13

enum CommunicationMode { MODE_USB, MODE_BLE };

void initButton(void);

CommunicationMode getCommunicationMode(void);

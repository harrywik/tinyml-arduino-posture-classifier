#include <ArduinoBLE.h>
#include "ble.h"

BLEService customService(BLE_SLAVE_UUID);
// BLECharacteristic(uuid, properties, value_size)
uint8_t permissions = BLERead | BLEWrite;
// TODO:
// calculate the max amount of data transmitted
uint8_t maxNumerOfBytes = 1; // This is just a test...
BLECharacteristic sensorCharacteristic(BLE_SLAVE_UUID, permissions, 1);


bool initBLE(void) {
	BLE.setAdvertisedService(customService);

	String name = "ArduinoBLE-";

	// Make unique
	String uuidString = String(BLE_SLAVE_UUID);
	name += uuidString.substring(0, 8);
	
	// Set name
	BLE.setLocalName(name.c_str()); 

  	// Set the advertised service UUID
  	BLE.setAdvertisedService(customService); 

  	// Add the characteristic to the service
  	customService.addCharacteristic(sensorCharacteristic); 
  
  	// Add the service to the BLE device
  	BLE.addService(customService); 

  	// Set the initial value for the characteristic
  	sensorCharacteristic.writeValue((uint8_t) 0x00); 

  	// --- Start Advertising ---
  	BLE.advertise();
	return true;
}

bool isBLEConnected() {
	BLEDevice central = BLE.central();
	return central && central.connected();
}

void BLESend(const String& msg) {
	if (!isBLEConnected()) return;
	// For simplicity, we assume the message fits in one characteristic write
	sensorCharacteristic.writeValue((const uint8_t*) msg.c_str(), msg.length());
}

bool BLEReceive(String &cmd) {
    BLEDevice central = BLE.central();
    if (!central) return false;

    if (sensorCharacteristic.written()) {
        cmd = String((char*)sensorCharacteristic.value(), sensorCharacteristic.valueLength());  
        return true;
    }
    return false;
}

bool BLESendModel(float* weights, size_t len) {
    const uint8_t* ptr = (uint8_t*)weights;
    size_t bytes = len * sizeof(float);

    while (bytes > 0) {
        size_t chunk = min(bytes, 20);
        sensorCharacteristic.writeValue(ptr, chunk);
        delay(10);

        ptr += chunk;
        bytes -= chunk;
    }

    return true;
}

bool BLEReceiveModel(float* weights, size_t len) {
    uint8_t* ptr = (uint8_t*)weights;
    size_t bytes = len * sizeof(float);
    size_t received = 0;

    while(received < bytes) {
        if(sensorCharacteristic.written()) {
            int chunk = sensorCharacteristic.valueLength();
            memcpy(ptr + received, sensorCharacteristic.value(), chunk);
            received += chunk;
        }
    }
    return true;
}

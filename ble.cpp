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
}

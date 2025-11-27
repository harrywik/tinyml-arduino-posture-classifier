#include <ArduinoBLE.h>
#include "ble.h"

BLEService customService(BLE_SLAVE_UUID);
// BLECharacteristic(uuid, properties, value_size)
uint_8 permissions = BLERead | BLEWrite;
// TODO:
// calculate the max amount of data transmitted
uint_8 maxNumerOfBytes = 1; // This is just a test...
BLECharCharacteristic sensorCharacteristic(BLE_SLAVE_UUID, permissions, 1);

BLE.setAdvertisedService(customService)

bool initBLE(void) {
	String name = "ArduinoBLE-";

	// Make unique
	name.append(BLE_SLAVE_UUID, 0, 8)
	// Set name
	BLE.setLocalName(name); 

  	// Set the advertised service UUID
  	BLE.setAdvertisedService(customService); 

  	// Add the characteristic to the service
  	customService.addCharacteristic(sensorCharacteristic); 
  
  	// Add the service to the BLE device
  	BLE.addService(customService); 

  	// Set the initial value for the characteristic
  	sensorCharacteristic.writeValue(0x00); 

  	// --- Start Advertising ---
  	BLE.advertise();
}

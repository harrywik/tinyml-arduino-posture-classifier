#include <ArduinoBLE.h>
#include "io.h"
#include "ble.h"
#include "esn.h"

BLEService customService(BLE_SLAVE_UUID);
uint8_t permissions = BLERead | BLEWrite | BLENotify;
BLECharacteristic sensorCharacteristic(BLE_CHARACTERISTIC_UUID, permissions, 64);

// Store connected peripheral for central mode
BLEDevice connectedPeripheral;

void deinitAsCentral(void) {
	if (BLE.connected()) {
		BLE.disconnect();
	}
	BLE.stopScan();
	BLE.end();
}

void deinitAsPeripheral(void) {
	if (BLE.connected()) {
		BLE.disconnect();
	}
	BLE.stopAdvertise();
	BLE.end();
}

bool startCentralService(uuid peripheralUUID) {
	BLE.begin();
	BLE.scanForUuid(peripheralUUID);
}

bool attemptConnectionToPeripheral(uuid peripheralUUID) {
    BLEDevice peripheral = BLE.available();
    if (!peripheral)
        return false;

    // Attempt connection
    if (!peripheral.connect()) {
        // If it fails, resume scanning
        BLE.scanForUuid(peripheralUUID);
        return false;
    }

    // Store the connected peripheral for later use
    connectedPeripheral = peripheral;

    // Discover attributes once after connection
    connectedPeripheral.discoverAttributes();

    // Now connected
    return true;
}

bool initBLE(void) {
	if (!BLE.begin())
		return false;

	String name = "id";

	// Make unique
	String uuidString = String(BLE_SLAVE_UUID);
	name += uuidString.substring(0, 4);
	
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

bool isBLEConnected(void) {
	BLEDevice central = BLE.central();
	return central && central.connected();
}

void readvertiseBLE(void) {
	BLE.advertise();
}

void BLESend(const String& msg) {
	if (!isBLEConnected()) return;
	// For simplicity, we assume the message fits in one characteristic write
	sensorCharacteristic.writeValue((const uint8_t*) msg.c_str(), msg.length());
}

bool BLEReceive(String &cmd) {
    BLEDevice central = BLE.central();
    if (!central) {
	readvertiseBLE();
	return false;
    }

    if (sensorCharacteristic.written()) {
        cmd = String((char*)sensorCharacteristic.value(), sensorCharacteristic.valueLength());  
        return true;
    }
    return false;
}

bool weightShareSend(WeightShareBLEMode mode, BLEMsgType type, uint8_t* data, size_t bytes) {
	uint8_t* ptr = data;
	size_t chunk;

	if (mode == WS_BLE_CENTRAL) {
		// Use stored connected peripheral (attributes already discovered)
		BLECharacteristic remoteChar = connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		if (!remoteChar) {
			return false;
		}

		delay(10);

		// Send type byte first
		remoteChar.writeValue((uint8_t*)&type, 1);
		delay(10);

		while (bytes > 0) {
			chunk = min(bytes, MAX_CHUNK_LENGTH);
			remoteChar.writeValue(ptr, chunk);
			delay(20);

			ptr += chunk;
			bytes -= chunk;
		}

		// Give peripheral extra time to process all chunks
		delay(100);
		return true;
	} 
	// mode == WS_BLE_PERIPHERAL
	sensorCharacteristic.writeValue((uint8_t*)&type, 1);
	delay(10);
	BLE.poll();

    	while (bytes > 0) {
        	chunk = min(bytes, MAX_CHUNK_LENGTH);
        	sensorCharacteristic.writeValue(ptr, chunk);
        	delay(10);
		BLE.poll();

		ptr += chunk;
		bytes -= chunk;
	}

	return true;
}

bool weightShareReceive(WeightShareBLEMode mode, BLEMsgType type, uint8_t* data, size_t bytes) {
	uint8_t* ptr = data;
	size_t received = 0;
	size_t chunk;

	if (mode == WS_BLE_CENTRAL) {
		// Use stored connected peripheral
		if (!connectedPeripheral) {
            		return false;
        }
		BLECharacteristic remoteChar = connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		if (!remoteChar) {
			return false;
		}
        
		while(!remoteChar.written()) { 
			BLE.poll();
		}
		size_t typeLen = remoteChar.valueLength();

		if (typeLen != 1|| remoteChar.value()[0] != type)
			return false;

		// correct type 
		while(received < bytes) {
			if(remoteChar.written()) {
				chunk = remoteChar.valueLength();
				memcpy(ptr + received, remoteChar.value(), chunk);
				received += chunk;
			}
			BLE.poll();
		}
		return true;
	} 
	// mode == WS_BLE_PERIPHERAL

	// check type of incoming
	while(!sensorCharacteristic.written()) {
		BLE.poll();
	}

	// Read the type byte
	if (sensorCharacteristic.valueLength() != 1)
		return false;

	uint8_t receivedType = sensorCharacteristic.value()[0];
	if (receivedType != type)
		return false;

	// correct type
	unsigned long startTime = millis();
	const unsigned long timeout = 10000; // 10 second timeout

	while(received < bytes) {
		if(sensorCharacteristic.written()) {
			chunk = sensorCharacteristic.valueLength();
			memcpy(ptr + received, sensorCharacteristic.value(), chunk);
			received += chunk;
			startTime = millis(); // Reset timeout on successful receive
		}
		BLE.poll();

		// Check for timeout
		if (millis() - startTime > timeout) {
			return false; // Timeout - didn't receive all data
		}
	}
	return true;
}

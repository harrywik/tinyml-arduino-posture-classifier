#include <ArduinoBLE.h>
#include "io.h"
#include "ble.h"
#include "esn.h"

BLEService customService(BLE_SLAVE_UUID);
uint8_t permissions = BLERead | BLEWrite | BLENotify;
BLECharacteristic sensorCharacteristic(BLE_CHARACTERISTIC_UUID, permissions, 64);

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
	if (!peripheral.hasService(peripheralUUID))
		return false;
	// Has correct UUID
	if (BLE.connected()) // this fuction expects 0 argument
		return true;
	// Otherwise retry
	BLE.scanForUuid(peripheralUUID);
	return false;
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
		BLEDevice peripheral = BLE.available();

		peripheral.connect();
		peripheral.discoverAttributes();
		BLECharacteristic remoteChar = peripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		delay(10);

		while (bytes > 0) {
			chunk = min(bytes, MAX_CHUNK_LENGTH);
			remoteChar.writeValue(ptr, chunk);
			delay(10);

			ptr += chunk;
			bytes -= chunk;
		}
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
		BLEDevice peripheral = BLE.available();
		// check type of incoming
		if (!peripheral) {
            return false;
        }
		BLECharacteristic remoteChar = peripheral.characteristic(BLE_CHARACTERISTIC_UUID);
        
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
		;
	}
	size_t typeLen = sensorCharacteristic.valueLength();

	if (typeLen != 1|| sensorCharacteristic.value()[0] != type)
		return false;

	// correct type 
	while(received < bytes) {
		if(sensorCharacteristic.written()) {
			chunk = sensorCharacteristic.valueLength();
			memcpy(ptr + received, sensorCharacteristic.value(), chunk);
			received += chunk;
		}
		BLE.poll();
	}
	return true;
}

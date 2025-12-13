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
	if (!BLE.begin()) return false;
	// BLE.scanForUuid(peripheralUUID);
	BLE.stopScan();
	BLE.scan();
	return true;
}

bool attemptConnectionToPeripheral(uuid peripheralUUID) {
    BLEDevice peripheral = BLE.available();
    if (!peripheral)
		//Serial.println("No peripheral available");
        return false;

    // Attempt connection
	String expectedPrefix = "id" + String(peripheralUUID).substring(0, 4);
	String name = peripheral.localName();

	if (name.length() == 0 || name != expectedPrefix) {
		// Not the correct peripheral
		// Serial.println("Peripheral name mismatch: " + name);
		return false;
	}	

	Serial.print("Found candidate: ");
	Serial.print(name);
	Serial.print(" addr=");
	Serial.println(peripheral.address());

	BLE.stopScan();
	delay(20);

    if (!peripheral.connect()) {
        // If it fails, resume scanning
        //BLE.scanForUuid(peripheralUUID);
		Serial.println("Failed to connect to peripheral");
		BLE.scan();
        return false;
    }

    // Store the connected peripheral for later use
    connectedPeripheral = peripheral;

	// Discover attributes
    if (!connectedPeripheral.discoverAttributes()) {
        Serial.println("discoverAttributes() failed");
        connectedPeripheral.disconnect();
        // BLE.scanForUuid(peripheralUUID);
        return false;
    }

    // IMPORTANT: wait until the remote characteristic is actually available
    unsigned long start = millis();
    while ((millis() - start) < 5000) {
        BLE.poll();

        BLECharacteristic remoteChar =
            connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

        if (remoteChar) {
            Serial.println("Remote characteristic discovered");
            return true;
        }

        delay(10);
    }

    Serial.println("Timeout waiting for remote characteristic");
    connectedPeripheral.disconnect();
    // BLE.scanForUuid(peripheralUUID);
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
	Serial.println("initBLE(): BLE.advertise() called");
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
		Serial.println("weightShareSend() as CENTRAL");
		BLECharacteristic remoteChar = connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		if (!remoteChar) {
			Serial.println("Remote characteristic not found");
			return false;
		}
		
		BLE.poll();
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

	// Simple timeout to avoid infinite blocking
    const unsigned long TYPE_TIMEOUT_MS  = 5000;
    const unsigned long DATA_TIMEOUT_MS  = 10000;

	if (mode == WS_BLE_CENTRAL) {
		// Use stored connected peripheral
		if (!connectedPeripheral || !connectedPeripheral.connected()) {
			Serial.println("Not connected to peripheral");
            return false;
        }
		BLECharacteristic remoteChar = connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		if (!remoteChar) {
			Serial.println("Remote characteristic not found");
			return false;
		}
        
		uint8_t header[1];
		bool gotType = false;
		unsigned long start = millis();

		while (!gotType && (millis() - start) < TYPE_TIMEOUT_MS) {
			int len = remoteChar.readValue(header, sizeof(header));
			if (len == 1) {
				Serial.println("Received type byte");
				gotType = true;
				break;
			}
			BLE.poll();
			delay(5);
		}

		if (!gotType || header[0] != type) {
			Serial.println("Failed to receive correct type byte");
			return false;
		}

        // 2) Read data chunks until we have 'bytes' bytes
        start = millis();
        while (received < bytes && (millis() - start) < DATA_TIMEOUT_MS) {
            uint8_t buf[MAX_CHUNK_LENGTH];
            int len = remoteChar.readValue(buf, sizeof(buf));

            if (len > 0) {
                size_t remaining = bytes - received;
                chunk = min((size_t)len, remaining);
                memcpy(ptr + received, buf, chunk);
                received += chunk;
                // reset timeout after progress
                start = millis();
            }
	
            BLE.poll();
            delay(5);
        }
		Serial.println("Received total bytes: " + String(received));
        return (received == bytes);
    }

	// mode == WS_BLE_PERIPHERAL

	// check type of incoming
	unsigned long start = millis();
	while(!sensorCharacteristic.written() && (millis() - start) < TYPE_TIMEOUT_MS) {
		BLE.poll();
	}

    if (!sensorCharacteristic.written()) {
        // timeout
		Serial.println("Timeout waiting for type byte");
        return false;
    }

	size_t typeLen = sensorCharacteristic.valueLength();

    if (typeLen != 1 || sensorCharacteristic.value()[0] != type)
		Serial.println("Received incorrect type byte");
        return false;

	start = millis();

	while(received < bytes && (millis() - start) < DATA_TIMEOUT_MS) {
        if (sensorCharacteristic.written()) {
            size_t available = sensorCharacteristic.valueLength();
            if (available > 0) {
                size_t remaining = bytes - received;
                chunk = min(available, remaining);
                memcpy(ptr + received, sensorCharacteristic.value(), chunk);
                received += chunk;
                // reset timeout after progress
                start = millis();
            }
        }
        BLE.poll();
    }
	Serial.println("Received total bytes: " + String(received));
	return (received == bytes);
}

#include <ArduinoBLE.h>
#include "io.h"
#include "ble.h"
#include "esn.h"

BLEService customService(BLE_SLAVE_UUID);
uint8_t permissions = BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify;
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

            // Subscribe to the characteristic if it supports notifications
            if (remoteChar.canSubscribe()) {
                if (!remoteChar.subscribe()) {
                    Serial.println("Failed to subscribe to characteristic");
                } else {
                    Serial.println("Subscribed to characteristic");

                    // Flush any stale notifications by polling and discarding updates
                    for (int i = 0; i < 10; i++) {
                        BLE.poll();
                        if (remoteChar.valueUpdated()) {
                            // Discard stale value
                            uint8_t dummy[64];
                            remoteChar.readValue(dummy, sizeof(dummy));
                            Serial.println("Flushed stale notification");
                        }
                        delay(10);
                    }
                }
            }

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
		// Serial.println("weightShareSend() as CENTRAL");
		BLECharacteristic remoteChar = connectedPeripheral.characteristic(BLE_CHARACTERISTIC_UUID);

		if (!remoteChar) {
			Serial.println("Remote characteristic not found");
			return false;
		}

		if (!remoteChar.canWrite()) {
			Serial.println("Remote characteristic is NOT writable");
			return false;
		}	
		
		BLE.poll();
		delay(10);

		// Send type byte first
		// remoteChar.writeValue((uint8_t*)&type, 1);
		// BLE.poll();
		// delay(10);

		if (!remoteChar.writeValue((uint8_t*)&type, 1)) {
			Serial.println("Failed to write type");
			return false;
		}

		BLE.poll();
		delay(100);

		while (bytes > 0) {
			chunk = min(bytes, MAX_CHUNK_LENGTH);
			if (!remoteChar.writeValue(ptr, chunk)) {
				Serial.println("Failed to write payload chunk");
				return false;
			}
			BLE.poll();
			delay(100);

			ptr += chunk;
			bytes -= chunk;
		}

		// Give peripheral extra time to process all chunks
		// delay(100);
		return true;
	} 
	// mode == WS_BLE_PERIPHERAL
	Serial.print("Peripheral sending type byte: ");
	Serial.println(type);
	sensorCharacteristic.writeValue((uint8_t*)&type, 1);
	BLE.poll();
	delay(100);

	int chunkCount = 0;
	size_t totalBytes = bytes;
	while (bytes > 0) {
		chunk = min(bytes, MAX_CHUNK_LENGTH);
		sensorCharacteristic.writeValue(ptr, chunk);
		BLE.poll();
		delay(100);

		ptr += chunk;
		bytes -= chunk;
		chunkCount++;
		Serial.print("Peripheral sent chunk ");
		Serial.print(chunkCount);
		Serial.print(": ");
		Serial.print(chunk);
		Serial.println(" bytes");
	}
	Serial.print("Peripheral sent total of ");
	Serial.print(chunkCount);
	Serial.print(" chunks, ");
	Serial.print(totalBytes);
	Serial.println(" bytes");

	return true;
}

bool weightShareReceive(WeightShareBLEMode mode, BLEMsgType type, uint8_t* data, size_t bytes) {
	uint8_t* ptr = data;
	size_t received = 0;
	size_t chunk;

	// Increased timeout to handle desynchronized entry
    const unsigned long TYPE_TIMEOUT_MS  = 60000;
    const unsigned long DATA_TIMEOUT_MS  = 60000;

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
			BLE.poll();
			delay(20);
			BLE.poll();

			// Check if value was updated (via notification or direct read)
			if (remoteChar.valueUpdated()) {
				int len = remoteChar.valueLength();
				if (len == 1) {
					remoteChar.readValue(header, sizeof(header));
					Serial.print("Received type byte: ");
					Serial.print(header[0]);
					Serial.print(" Expected: ");
					Serial.println(type);
					gotType = true;
					break;
				}
			}
		}

		if (!gotType || header[0] != type) {
			Serial.println("Failed to receive correct type byte");
			return false;
		}

        // 2) Read data chunks until we have 'bytes' bytes
        start = millis();
        int chunkCount = 0;
        while (received < bytes && (millis() - start) < DATA_TIMEOUT_MS) {
            BLE.poll();

            // Check if value was updated (via notification)
            if (remoteChar.valueUpdated()) {
                uint8_t buf[MAX_CHUNK_LENGTH];
                int len = remoteChar.valueLength();

                if (len > 0) {
                    remoteChar.readValue(buf, len);
                    size_t remaining = bytes - received;
                    chunk = min((size_t)len, remaining);
                    memcpy(ptr + received, buf, chunk);
                    received += chunk;
                    chunkCount++;
                    Serial.print("Chunk ");
                    Serial.print(chunkCount);
                    Serial.print(": ");
                    Serial.print(chunk);
                    Serial.print(" bytes, total: ");
                    Serial.println(received);
                    // reset timeout after progress
                    start = millis();
                }
            }
            delay(20);
        }
        Serial.print("Received ");
        Serial.print(chunkCount);
        Serial.print(" chunks, ");
		Serial.println("Received total bytes: " + String(received));
        return (received == bytes);
    }

	// mode == WS_BLE_PERIPHERAL
    BLEDevice central = BLE.central();
    // if (!central || !central.connected()) {
    //     Serial.println("Peripheral not connected to any central");
    //     return false;
    // }

	// check type of incoming
	unsigned long start = millis();
	bool gotType = false;
	size_t lastLen = 0;

	while(!gotType && (millis() - start) < TYPE_TIMEOUT_MS) {
		BLE.poll();
		delay(5);

		// refresh central state
        if (!central|| !central.connected()) {
            Serial.println("Central disconnected while waiting type");
            return false;
        }

		// Check if value has been updated (either .written() or value length changed)
		size_t currentLen = sensorCharacteristic.valueLength();
		if (sensorCharacteristic.written() || (currentLen > 0 && currentLen != lastLen)) {
			if (currentLen == 1 && sensorCharacteristic.value()[0] == type) {
				gotType = true;
				break;
			} else if (currentLen == 1) {
				Serial.println("Received incorrect type byte");
				return false;
			}
			lastLen = currentLen;
		}
    }

    if (!gotType) {
        // timeout
		Serial.println("Timeout waiting for type byte");
        return false;
    }

	start = millis();
	lastLen = 1; // We just read the type byte (length 1)

	while(received < bytes && (millis() - start) < DATA_TIMEOUT_MS) {
		BLE.poll();
		delay(5);

		// Check if value has been updated (either .written() or value length changed)
		size_t currentLen = sensorCharacteristic.valueLength();
        if (sensorCharacteristic.written() || (currentLen > 0 && currentLen != lastLen)) {
            if (currentLen > 0) {
                size_t remaining = bytes - received;
                chunk = min(currentLen, remaining);
                memcpy(ptr + received, sensorCharacteristic.value(), chunk);
                received += chunk;
                lastLen = currentLen;
                // reset timeout after progress
                start = millis();
            }
        }
    }
	Serial.println("Received total bytes: " + String(received));
	return (received == bytes);
}

#include <ArduinoBLE.h>
#include "ble.h"
#include "esn.h"
#include "io.h"

BLEService customService(BLE_SLAVE_UUID);
uint8_t permissions = BLERead | BLEWrite | BLENotify;
BLECharacteristic sensorCharacteristic(BLE_CHARACTERISTIC_UUID, permissions, 64);

static BLEDevice peerDevice; // Store paired device
static bool isCentral = false; // Mark whether current role is Central or Peripheral
static bool peerConnected = false; // Mark point-to-point connection status
static bool weightSharingActive = false; // Mark whether weight sharing is in progress

const String KNOWN_PEER_MACS[] = {
    "AA:BB:CC:DD:EE:FF", // Need to replace with actual MAC 1
    "11:22:33:44:55:66", // Need to replace with actual MAC 2
};
const int NUM_KNOWN_PEERS = sizeof(KNOWN_PEER_MACS) / sizeof(KNOWN_PEER_MACS[0]);

// Protocol message types
enum BLEMsgType {
  MSG_TYPE_CMD = 0,      // Regular command
  MSG_TYPE_BATCH_COUNT = 1, // Batch count
  MSG_TYPE_WEIGHTS = 2,     // Weight data
  MSG_TYPE_DONE = 3         // Exchange completion signal
};

// Attempt to connect to the first known peer that is not the local device (Central)
bool connectToAnyKnownPeer() {
    Serial.println("Stopping peripheral advertising...");
    BLE.stopAdvertise();

    // Get the local device's address to avoid connecting to itself
    String localAddress = BLE.address();
    Serial.print("Local device address: ");
    Serial.println(localAddress);

    // Iterate through known peers
    for (int i = 0; i < NUM_KNOWN_PEERS; i++) {
        String peerMAC = KNOWN_PEER_MACS[i];

        // Skip if this is the local address
        if (peerMAC.equalsIgnoreCase(localAddress)) {
            Serial.print("Skipping local address: ");
            Serial.println(peerMAC);
            continue;
        }

        Serial.print("Attempting to connect to peer: ");
        Serial.println(peerMAC);

        BLEAddress targetAddress(peerMAC.c_str());

        // Scan for specified address
        if (!BLE.scanForAddress(targetAddress, 5000)) { // 5 second timeout per address
            Serial.print("Peer scan failed or timeout for: ");
            Serial.println(peerMAC);
            continue; // Try the next peer
        }

        peerDevice = BLE.available();
        if (peerDevice) {
            Serial.print("Peer found (");
            Serial.print(peerDevice.address());
            Serial.println("), connecting...");
            if (peerDevice.connect()) {
                Serial.println("Connected to peer as Central!");
                peerConnected = true;
                isCentral = true;
                return true; // Successfully connected
            } else {
                Serial.print("Failed to connect to peer: ");
                Serial.println(peerMAC);
                // Continue to try the next peer
            }
        } else {
             Serial.print("Peer not found during scan for: ");
             Serial.println(peerMAC);
             // Continue to try the next peer
        }
    }

    // If loop finishes without connecting, all peers were tried and failed
    Serial.println("Failed to connect to any known peer.");
    BLE.advertise(); // Restore advertising
    weightSharingActive = false;
    return false;
}

// Wait for peer connection (Peripheral) or become Central after timeout
bool waitForPeerOrBecomeCentral() {
    unsigned long startTime = millis();
    const unsigned long timeout = 10000; // 10 second wait time
    Serial.println("Waiting for peer connection (Peripheral mode)...");

    while (millis() - startTime < timeout) {
        BLEDevice central = BLE.central();
        if (central) {
            Serial.println("Peer connected as Central!");
            peerDevice = central;
            peerConnected = true;
            isCentral = false; // I am Peripheral in this p2p link
            BLE.stopAdvertise(); // Stop advertising as connection is established
            return true;
        }
        delay(10);
    }

    Serial.println("Timeout waiting for peer, becoming Central...");
    return connectToAnyKnownPeer();
}

bool initBLE(void) {
	if (!BLE.begin())
		return false;
	BLE.setAdvertisedService(customService);

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

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


bool performWeightSharing(uint16_t localCount) { // Accept local count as argument
    if (!peerConnected) {
        Serial.println("Cannot share weights: Not connected to peer.");
        return false;
    }

    uint16_t remoteCount = 0; // To store the count received from the peer

    Serial.print("Local training count (e.g., samples/batches): ");
    Serial.println(localCount);

    if (isCentral) {
        // Central sends its local count first
        if (!sendBLEProtocol(MSG_TYPE_BATCH_COUNT, (uint8_t*)&localCount, sizeof(localCount))) {
            Serial.println("Failed to send local count.");
            return false;
        }
        // Central waits to receive peer's count
        if (!receiveBLEProtocol(MSG_TYPE_BATCH_COUNT, (uint8_t*)&remoteCount, sizeof(remoteCount))) {
            Serial.println("Failed to receive remote count.");
            return false;
        }
    } else { // isPeripheral
        // Peripheral receives peer's count first
        if (!receiveBLEProtocol(MSG_TYPE_BATCH_COUNT, (uint8_t*)&remoteCount, sizeof(remoteCount))) {
            Serial.println("Failed to receive remote count.");
            return false;
        }
        // Peripheral sends its local count afterwards
        if (!sendBLEProtocol(MSG_TYPE_BATCH_COUNT, (uint8_t*)&localCount, sizeof(localCount))) {
            Serial.println("Failed to send local count.");
            return false;
        }
    }

    Serial.print("Remote training count: ");
    Serial.println(remoteCount);

    // --- Exchange Weights ---
    float localWeights[OUTPUT_SIZE][RESERVOIR_SIZE];
    float remoteWeights[OUTPUT_SIZE][RESERVOIR_SIZE];

    // Get local weights from the global 'esn' instance
    memcpy(localWeights, esn.W_out, sizeof(localWeights));

    if (isCentral) {
        // Central sends its weights
        if (!sendBLEProtocol(MSG_TYPE_WEIGHTS, (uint8_t*)localWeights, sizeof(localWeights))) {
             Serial.println("Failed to send local weights.");
             return false;
        }
        // Central receives peer's weights
        if (!receiveBLEProtocol(MSG_TYPE_WEIGHTS, (uint8_t*)remoteWeights, sizeof(remoteWeights))) {
             Serial.println("Failed to receive remote weights.");
             return false;
        }
    } else { // isPeripheral
        // Peripheral receives peer's weights
        if (!receiveBLEProtocol(MSG_TYPE_WEIGHTS, (uint8_t*)remoteWeights, sizeof(remoteWeights))) {
             Serial.println("Failed to receive remote weights.");
             return false;
        }
        // Peripheral sends its weights
        if (!sendBLEProtocol(MSG_TYPE_WEIGHTS, (uint8_t*)localWeights, sizeof(localWeights))) {
             Serial.println("Failed to send local weights.");
             return false;
        }
    }

    // Perform FedAvg: Calculate weighted average of weights
    uint32_t total_count = (uint32_t)localCount + remoteCount; // Use uint32_t to prevent overflow if needed
    if (total_count > 0) {
        for (int i = 0; i < OUTPUT_SIZE; i++) {
            for (int j = 0; j < RESERVOIR_SIZE; j++) {
                // FedAvg formula: W_avg = (n_local * W_local + n_remote * W_remote) / (n_local + n_remote)
                esn.W_out[i][j] = (float)(localCount * localWeights[i][j] + remoteCount * remoteWeights[i][j]) / (float)total_count;
            }
        }
        Serial.println("Weights aggregated using FedAvg successfully.");
    } else {
        Serial.println("Warning: Total count is 0, cannot aggregate weights.");
        return false; // Or handle this specific case as needed
    }

    // Send completion signal
    sendBLEProtocol(MSG_TYPE_DONE, (uint8_t*)"", 0);

    // Disconnect and restore peripheral mode
    peerDevice.disconnect();
    peerConnected = false;
    isCentral = false;
    weightSharingActive = false;
    Serial.println("Disconnected from peer, restarting peripheral advertising...");
    BLE.advertise(); // Restart advertising

    return true;
}

bool sendBLEProtocol(BLEMsgType type, uint8_t* data, size_t len) {
    if (!peerConnected) return false;
    // Message format: [type_byte][data...]
    uint8_t packet[65]; // 64 data + 1 type byte
    if (len > 64) {
        Serial.println("Error: Data too large for single packet.");
        return false;
    }
    packet[0] = (uint8_t)type;
    memcpy(packet + 1, data, len);
    peerDevice.write(sensorCharacteristic, packet, len + 1);
    return true;
}

bool receiveBLEProtocol(BLEMsgType type, uint8_t* data, size_t expected_len) {
    if (!peerConnected) return false;
    if (sensorCharacteristic.written()) {
        uint8_t* value = (uint8_t*)sensorCharacteristic.value();
        size_t value_len = sensorCharacteristic.valueLength();
        if (value_len > 0 && value[0] == (uint8_t)type) {
            // Copy payload (skip type byte)
            size_t payload_len = min(expected_len, value_len - 1);
            memcpy(data, value + 1, payload_len);
            return true;
        }
    }
    return false;
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

import asyncio
import sys
import argparse
from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError
from bleak.backends.device import BLEDevice

# --- Configuration ---
# The UUIDs are derived directly from the Arduino code.
SERVICE_UUID = "9112C615-F25C-41D1-8292-AA39BD2E3387"
COMM_CHARACTERISTIC_UUID = "4ADFA629-DC24-4C30-B65A-4950FB6A581E"

# Global flag to control the main loop
is_running = True

def notification_handler(sender: int, data: bytearray):
    """
    Handler for incoming notification data (sent by Arduino's BLESend function).
    """
    try:
        # Attempt to decode the byte array to a UTF-8 string
        decoded_data = data.decode('utf-8').strip()
        print(f"\n[IN] << {decoded_data}")
        # Re-display the prompt after the incoming data
        sys.stdout.write("> ")
        sys.stdout.flush()
    except UnicodeDecodeError:
        # Print raw bytes if decoding fails
        print(f"\n[IN] << Raw Data: {data.hex()}")
        sys.stdout.write("> ")
        sys.stdout.flush()

async def user_input_handler(client: BleakClient):
    """
    Handles continuous input from the terminal and writes it to the BLE device
    (read by Arduino's BLEReceive function).
    """
    global is_running
    print("\n--- Terminal Ready ---")
    print(f"Connected to {client.address}. Type 'exit' to quit.")
    print("----------------------")

    while is_running:
        try:
            sys.stdout.write("> ")
            sys.stdout.flush()

            # Wait for user input without blocking the asyncio loop
            user_input = await asyncio.to_thread(sys.stdin.readline)
            user_input = user_input.strip()

            if user_input.lower() == 'exit':
                is_running = False
                break

            if user_input:
                # Encode the string to bytes for transmission
                data_to_send = user_input.encode('utf-8')

                # Write the data to the characteristic
                await client.write_gatt_char(COMM_CHARACTERISTIC_UUID, data_to_send, response=True)
                print(f"[OUT] >> Sent: {user_input}")

        except Exception as e:
            if is_running: # Only report error if we didn't initiate the shutdown
                print(f"\n[ERROR] An error occurred during input/write: {e}")
            is_running = False
            break

async def connect_and_interact(address: str):
    """Attempts to connect to a single address and start the terminal."""
    global is_running
    
    print(f"Attempting connection to: {address}")
    
    try:
        async with BleakClient(address) as client:
            if not client.is_connected:
                print(f"[FAIL] Could not connect to {address}. Trying next device...")
                return False

            print(f"[SUCCESS] Connected to {address}!")

            # 1. Start listening for notifications
            print(f"Starting notifications on {COMM_CHARACTERISTIC_UUID}...")
            await client.start_notify(COMM_CHARACTERISTIC_UUID, notification_handler)

            # 2. Run the terminal input handler concurrently
            await user_input_handler(client)

            # 3. Stop notifications before disconnecting
            print(f"\nStopping notifications on {COMM_CHARACTERISTIC_UUID}...")
            await client.stop_notify(COMM_CHARACTERISTIC_UUID)
            return True # Successful connection and interaction
            
    except BleakError as e:
        print(f"\n[ERROR] Bleak error connecting to {address}: {e}")
        return False
    except Exception as e:
        print(f"\n[ERROR] Unexpected error connecting to {address}: {e}")
        return False

async def scan_and_connect():
    """
    Scans for devices, uses set difference to find the new Arduino, filters by UUID,
    and attempts to connect to the discovered addresses.
    """
    
    # 1. First Scan (Baseline)
    print("--- 1/4: Scanning for 5s (Arduino OFF) to get baseline noise...")
    dev0 = await BleakScanner.discover(timeout=5.0)
    
    # Pause for user to activate the Arduino
    _ = input("--- ACTION: Please power on your Arduino (or ensure it's advertising) and press Enter to continue...")
    
    # 2. Second Scan (With Arduino ON)
    print("--- 2/4: Scanning for 5s (Arduino ON) to find new devices...")
    dev1 = await BleakScanner.discover(timeout=5.0)

    # 3. Calculate Set Difference (dev1 - dev0)
    # Isolate new devices that appeared after the user action.
    new_devices: list[BLEDevice] = list(set(dev1) - set(dev0))

    if not new_devices:
        print("\n[FAIL] No new devices were found in the second scan (set difference was empty).")
        print("Please ensure your Arduino is advertising and try again.")
        return

    # 4. Filter by Service UUID (with safe access and fallback)
    target_devices: list[BLEDevice] = []
    required_uuid = SERVICE_UUID.lower()
    
    for d in new_devices:
        # Use getattr() for safe access. If 'advertisement_data' is missing, adv_data becomes None, preventing crash.
        adv_data = getattr(d, 'advertisement_data', None)
        
        if adv_data:
            # Check the list of 128-bit services advertised by the device
            advertised_uuids = [s.lower() for s in adv_data.service_uuids]
            
            if required_uuid in advertised_uuids:
                target_devices.append(d)
        
    if not target_devices:
        print("\n[FAIL] Found new devices, but none advertised the required SERVICE_UUID.")
        print("This often happens when the underlying Bluetooth adapter fails to capture the full advertisement packet.")
        
        # Robust Fallback: Connect to the first new device found, trusting the set difference alone.
        for i, dev in enumerate(new_devices):
            print(f"[INFO] Connecting to the device #{i + 1} found based on MAC address: {dev.address}")
            # Call the connection handler directly with the first device's address
            connection_successful = await connect_and_interact(dev.address)
            
        if not connection_successful:
             print("[FAIL] Connection attempt failed even with fallback. Check device power and proximity.")
        return


    print("\n--- 4/4: Found Target Devices (Filtered by Service UUID) ---")
    for d in target_devices:
        print(f"Address: {d.address} | Name: {d.name or 'N/A'} (TARGET)")
    print("-------------------------------------------------------\n")
    
    # 5. Attempt to connect to the filtered devices one by one (if filtering was successful)
    print(f"Attempting to connect to {len(target_devices)} filtered device(s)...")
    
    connection_successful = False
    for device in target_devices:
        if await connect_and_interact(device.address):
            connection_successful = True
            break
            
    if not connection_successful:
        print("\n[FAIL] Could not establish a connection with any of the filtered devices.")
        print("This may be due to a timing issue or the device requiring bonding.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="BLE Terminal Client for Arduino.")
    parser.add_argument(
        '--address', 
        type=str, 
        help="The known MAC address of the BLE peripheral (e.g., AA:BB:CC:DD:EE:FF). Skips scanning if provided.",
        default=None
    )
    args = parser.parse_args()

    try:
        if args.address:
            # If address is provided, skip scanning and connect directly
            print(f"MAC address provided: {args.address}. Attempting direct connection...")
            asyncio.run(connect_and_interact(args.address))
        else:
            # Otherwise, run the full scan and discovery process
            asyncio.run(scan_and_connect())
    except KeyboardInterrupt:
        print("\n[INFO] User interrupted the program.")
    except Exception as e:
        print(f"[CRITICAL ERROR] Main loop failed: {e}")

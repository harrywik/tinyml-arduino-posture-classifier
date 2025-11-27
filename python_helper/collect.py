import argparse
import serial
import time
import csv
from pathlib import Path
from helpers import (
    send_line, 
    read_line,
    append_or_create_csv
)

def collect_cycle(ser, csv_path: str, n_features: int) -> bool:
    label: int
    while True:
        goahead = input("Ready to collect? (y/n): ").strip().lower()

        if goahead == "n":
            print("\nUser unready to collect data. Stopping.")
            return False 

        if goahead == "y":
            label = int(input("Enter label (0=SITTING, 1=STANDING, 2=ACTIVE): ").strip())
            break

    assert label in [0, 1, 2], "Bad label"
    send_line(ser, "COLLECT")

    # --- Data Collection Loop ---
    rows = []
    header = [f"f{i}" for i in range(n_features)]
    header.append("label")

    while True:
        line = read_line(ser)
        if not line:
            continue
        begin = "<row>"
        end = "</row>"
        if line == "</data>":
            # End of this iteration
            # Write to csv
            append_or_create_csv(Path(csv_path), header, rows)
            return True 
        elif line == "<data>":
            start = True
        elif line.startswith(begin) and start:
            feature_str = line[len(begin):-len(end)]
            row = [float(f) for f in feature_str.split(",")]
            row.append(label)
            rows.append(row)

def collect_features(device, baudrate, nfeats, csv_name):
    ser = serial.Serial(device, baudrate, timeout=0.1)
    time.sleep(2)
    print("Starting feature collection. Press Ctrl+C to stop.")

    try:
        while collect_cycle(ser, csv_name, nfeats):
            pass 

    except KeyboardInterrupt:
        print("\nFeature collection stopped by user.")

    finally:
        ser.close()
        print("Serial connection closed.")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Collect data from Arduino Nano 33 BLE Sense Lite.",
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument(
        '--device', 
        required=True,
        type=str,
        help='The device to communicate with (e.g. /dev/ttyACM0).'
    )

    parser.add_argument(
        '--baudrate', 
        type=int, 
        default=9600,
        help='The communication speed. (Default: 9600)'
    )

    parser.add_argument(
        '--nfeats', 
        type=int, 
        default=12,
        help='The number of features must match the arduino.. (Default: 12)'
    )

    parser.add_argument(
        '--csv', 
        type=str, 
        default="IMU_train.csv",
        help='The csv file to write output to. (Default: IMU_train.csv)'
    )


    args = parser.parse_args()

    collect_features(args.device, args.baudrate, args.nfeats, args.csv)


import argparse
import serial
import csv
import time
from helpers import ( 
    send_line, 
    read_line,
    parse_predictions
)
from pathlib import Path

def stream_data(
        device: str, 
        csv_file: str, 
        write_file: str,
        baudrate: int, 
        mode: str, 
        nfeats: int, 
        winlen: int
):
    ser = serial.Serial(device, baudrate, timeout=0.5)
    time.sleep(2)  # Arduino reset

    send = "TRAIN" if mode == "train" else "VALIDATE"
    short = "TRAIN" if mode == "train" else "VAL"

    batch = 0

    try:
        with open(csv_file, newline='\n') as csvfile:
            n_rows = sum(1 for line in csvfile)
            csvfile.seek(0) # reset after counting lines
            reader = csv.reader(csvfile, delimiter=',')
            y_true = []

            # Send command
            print(f"Sending [CMD={short}]")
            send_line(ser, send)

            for i, row in enumerate(reader):
                if i == 0:
                    # Headers
                    continue

                # Send it over
                y_true.append(row[-1])
                send_line(ser, ",".join(row))

                # Wait a bit
                time.sleep(0.01)

                # Window is full
                # Get predictions
                if i % winlen == 1 and i != 1:
                    parse_predictions(Path(write_file), ser, y_true)
                    batch += 1
                    print(f"Batch {batch} has been processed. Ctrl+C to terminate")
                    print(f"Sending [CMD={short}]")
                    # Reset y_true before collecting more
                    y_true = [] 
                    send_line(ser, send)

            # Process the final one
            parse_predictions(Path(write_file), ser, y_true)
            batch += 1
            print(f"Batch {batch} has been processed.")
            print("DONE")
    finally:
        ser.close()
        print("\nSerial connection closed.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Stream train data to the Arduino Nano 33 BLE Sense Lite.",
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument(
        '--device', 
        required=True,
        type=str,
        help='The device to communicate with (e.g. /dev/ttyACM0).'
    )

    parser.add_argument(
        '--csv', 
        type=str, 
        default="IMU_train.csv",
        help='The csv file to get the input from. (Default: IMU_train.csv)'
    )

    parser.add_argument(
        "--write",
        type=str,
        default="predictions.csv",
        help="Where to write the predictions to. (Default: predictions.csv)"
    )

    parser.add_argument(
        '--nfeats', 
        type=int, 
        default=12,
        help='The number of features must match the arduino.. (Default: 12)'
    )
    
    parser.add_argument(
        '--winlen', 
        type=int, 
        default=64,
        help='The size of the feature window the arduino accepts. (Default: 64)'
    )

    parser.add_argument(
        '--baudrate', 
        type=int, 
        default=9600,
        help='The communication speed. (Default: 9600)'
    )

    parser.add_argument(
        "--mode",
        choices=["train", "val"],
        default="train",
        help="The type of data to stream. Options: train, val. (Default: train)"
    )


    args = parser.parse_args()

    stream_data(args.device, args.csv, args.write, args.baudrate, args.mode, args.nfeats, args.winlen)


import argparse
import serial
import time
import csv
from helpers import send_line, read_line

NUM_FEATURES = 12  # must match Arduino

LABEL_MAP = {'SITTING': 0, 'STANDING': 1, 'LYING': 2, 'ACTIVE': 3}

def collect_features(device, baudrate, nfeats, csv_name):
    ser = serial.Serial(device, baudrate, timeout=0.1)
    time.sleep(2)  # wait for Arduino reset
    print("Starting feature collection. Press Ctrl+C to stop.")

    with open(csv_name, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([f'f{i}' for i in range(nfeats)] + ['label'])

        loop(writer, ser)

def loop(writer, ser):
        try:
            label = -1;
            while True:
                goahead = input("Ready to collect? (y/n): ").strip().lower()
                if goahead == "n":
                    raise KeyboardInterrupt("User unready to collect data")

                label = int(input("Enter label (0=SITTING, 1=STANDING, 2=ACTIVE): ").strip())

                send_line("COLLECT")

            assert label in [0, 1, 2], "Bad label"

            start = False

            while True:
                line = read_line(ser)

                begin = "<row>"
                end = "</row>"

                if line == "</data>":
                    # Ready for next label and data stream
                    loop(writer, ser)
                elif line == "<data>":
                    start = True
                elif line.startswith(begin) and start:
                    feature_str = line[len(begin):-len(end)]
                    features = [float(f) for f in feature_str.split(",")]
                    writer.writerow(features + [label])

        except KeyboardInterrupt:
            print("Feature collection stopped by user.")
        finally:
            ser.close()


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


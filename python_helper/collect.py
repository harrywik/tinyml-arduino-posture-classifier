import serial
import time
import csv
from helpers import send_line, read_line


SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
FEATURES_CSV = 'imu_features.csv'
NUM_FEATURES = 12  # must match Arduino

LABEL_MAP = {'SITTING': 0, 'STANDING': 1, 'LYING': 2, 'ACTIVE': 3}

def collect_features():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2)  # wait for Arduino reset
    print("Starting feature collection. Press Ctrl+C to stop.")

    with open(FEATURES_CSV, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([f'f{i+1}' for i in range(NUM_FEATURES)] + ['label'])

        try:
            while True:
                line = read_line(ser)
                if line:
                    # Parse feature vector
                    if line.startswith("FEATURE:"):
                        feature_str = line.split(":")[1]
                        features = [float(f) for f in feature_str.split(",")]
                        writer.writerow(features + ["insert_label"])

        except KeyboardInterrupt:
            print("Feature collection stopped by user.")
            ser.close()

if __name__ == "__main__":
    collect_features()


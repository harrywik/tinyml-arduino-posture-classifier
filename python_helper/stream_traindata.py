import serial
import csv
import time
from helpers import send_line, read_line

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
CSV_FILE = 'imu_features.csv'
NUM_FEATURES = 12

SLEEP_BETWEEN_CMD = 0.02

def stream_training_data(csv_file):
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    time.sleep(2)  # Arduino reset
    print("Starting training data stream...")

    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Extract features and label
            features = [float(row[f'f{i+1}']) for i in range(NUM_FEATURES)]
            label = int(row['label'])

            # Send feature vector
            feature_line = "FEATURE:" + ",".join(f"{f:.6f}" for f in features)
            send_line(ser, feature_line)

            # Wait for READY from Arduino
            while True:
                line = read_line(ser)
                if line:
                    if line.strip() == "READY":
                        send_line(ser, f"LABEL:{label}")
                        break
                time.sleep(0.01)

    print("All training data streamed. Sending TRAIN command...")
    send_line(ser, "TRAIN")
    print("Training command sent. Closing serial port.")
    ser.close()

if __name__ == "__main__":
    stream_training_data(CSV_FILE)


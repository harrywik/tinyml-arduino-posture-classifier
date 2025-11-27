import time
import csv
from pathlib import Path

SLEEP_BETWEEN_CMD=0.2

def send_line(ser, line):
    ser.write((line + '\n').encode())
    time.sleep(SLEEP_BETWEEN_CMD)

def read_line(ser):
    if ser.in_waiting:
        return ser.readline().decode().strip()

def append_or_create_csv(file_path: Path, data_rows: list, n_features: int = 12):
    header = [f"f{i}" for i in range(n_features)]
    header.append("label")

    file_exists = file_path.exists()

    if not file_exists:
        print(f"File '{file_path}' not found. Creating and writing header.")
        with file_path.open('w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(header)

    with file_path.open('a', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerows(data_rows)

    print(f"Data successfully appended to {str(file_path)}.") 

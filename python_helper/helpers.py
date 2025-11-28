import time
import csv
from pathlib import Path

SLEEP_BETWEEN_CMD=0.2

def send_line(ser, line, sleep=True):
    ser.write((line + '\n').encode())
    if sleep:
        time.sleep(SLEEP_BETWEEN_CMD)

def read_line(ser) -> str | None:
    if ser.in_waiting:
        return ser.readline().decode().strip()

def append_or_create_csv(file_path: Path, header: str, data_rows: list):
    file_exists = file_path.exists()

    if not file_exists:
        print(f"File '{file_path}' not found. Creating and writing header.")
        with file_path.open('w', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(header)

    with file_path.open('a', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerows(data_rows)

    print(f"Data successfully appended to {str(file_path)}.") 

def parse_predictions(outfile: Path, serial, y_true: list[int] | None = None):
    if y_true:
        header = ["mode", "y_hat", "y_true"]
    else:
        header = ["mode", "y_hat"]

    start = False
    i: int = 0
    rows = []
    e = 0
    while True:
        line = read_line(serial)
        if not line:
            e += 1
            # No data
            if e > 5:
                # Kill it
                break
            # Try again
            else:
                continue
        e = 0
        print(line)

        option: str
        if line == "<result>":
            start = True
            continue
        if line == "</result>":
            append_or_create_csv(outfile, header, rows)
            return

        if not start:
            continue

        if line.startswith("<train>"):
            option = "train"
        elif line.startswith("<pred>"):
            option = "pred"
        elif line.startswith("<val>"):
            option = "val"

        label = int(line[len(option) + 2:-( len(option) + 3)])

        if y_true:
            rows.append([option, label, y_true[i]])
            i += 1
        else:
            rows.append([option, label])


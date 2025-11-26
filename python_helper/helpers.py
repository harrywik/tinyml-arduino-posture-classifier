def send_line(ser, line):
    ser.write((line + '\n').encode())
    time.sleep(SLEEP_BETWEEN_CMD)

def read_line(ser):
    if ser.in_waiting:
        return ser.readline().decode().strip()
    return None

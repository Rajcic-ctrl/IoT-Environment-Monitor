import json
import serial

SERIAL_PORT = "COM3"
BAUD_RATE = 115200

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

print(f"Connected to {SERIAL_PORT}")

while True:
    line = ser.readline().decode("utf-8").strip()

    if not line:
        continue

    print("RAW:", line)

    try:
        data = json.loads(line)
    except json.JSONDecodeError:
        print("Invalid JSON")
        continue

    print(
        f"Temperature: {data['temperature']} C | "
        f"Humidity: {data['humidity']}% | "
        f"Light: {data['light']}% | "
        f"Status: {data['status']}"
    )
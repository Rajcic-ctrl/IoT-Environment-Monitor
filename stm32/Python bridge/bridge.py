import json
import serial
import paho.mqtt.client as mqtt
from datetime import datetime, timezone

SERIAL_PORT = "COM3"
BAUD_RATE = 115200

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "iot/environment"

REQUIRED_FIELDS = ["temperature", "humidity", "light", "status"]

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2) # type: ignore
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()

print("Bridge started.")

while True:
    try:
        line = ser.readline().decode("utf-8").strip()

        if not line:
            continue

        try:
            data = json.loads(line)
        except json.JSONDecodeError:
            print("Invalid JSON:", line)
            continue

        if not all(field in data for field in REQUIRED_FIELDS):
            print("Incomplete data:", data)
            continue

        data["timestamp"] = datetime.now(timezone.utc).isoformat()

        print(data)

        client.publish(MQTT_TOPIC, json.dumps(data))

    except serial.SerialException as error:
        print("Serial connection error:", error)
        break
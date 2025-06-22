import serial
import time
import re

com = serial.Serial('/dev/ttyUSB1', 115200, timeout=1)
pattern = re.compile(r'([a-zA-Z]+)=(-?\d+\.\d+)')


def fetch_stream():
    try:
        while True:
            line = com.readline().decode().strip() 
            if not line:
                time.sleep(0.1)
                continue
            matches = pattern.findall(line)
            row = {key: float(value) for key, value in matches}
            yield row

    except KeyboardInterrupt:
        pass
    finally:
        com.close()


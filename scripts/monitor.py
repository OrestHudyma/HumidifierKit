import sys
import time
import serial

port = serial.Serial(None, 115200, timeout=1)
port.dtr = False
port.rts = False
port.port = sys.argv[1] if len(sys.argv) > 1 else 'COM3'
with port:
    port.write(b'status\n')
    deadline = time.monotonic() + 25
    while time.monotonic() < deadline:
        line = port.readline().decode(errors='replace').strip()
        if line:
            print(line, flush=True)

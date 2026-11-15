"""USB bench check: exercises all eight combinations; always returns outputs OFF.

Run only with the bare board / appropriate test loads. This reads GPIO levels,
not relay contacts. Tank input and physical button need a separate bench check.
"""
import itertools
import re
import sys
import time
import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
with serial.Serial(port, 115200, timeout=0.3) as device:
    device.dtr = False
    device.rts = False
    time.sleep(1)

    def command(text):
        device.reset_input_buffer()
        device.write((text + "\n").encode())
        deadline = time.monotonic() + 3
        lines = []
        while time.monotonic() < deadline:
            line = device.readline().decode(errors="replace").strip()
            if line:
                lines.append(line)
            if line.startswith("STATUS "):
                return line
        raise RuntimeError(f"No status after {text}: {lines}")

    print(command("status"))
    try:
        for states in itertools.product((0, 1), repeat=3):
            for name, state in zip(("power", "backlight", "max_power"), states):
                response = command(f"{name} {'on' if state else 'off'}")
            actual = tuple(map(int, re.search(r"gpio=(\d),(\d),(\d)", response).groups()))
            assert actual == states, (states, response)
            print(f"PASS {states}: {response}")
    finally:
        for name in ("power", "backlight", "max_power"):
            command(f"{name} off")
    print("PASS: all eight independent output combinations; final outputs OFF")

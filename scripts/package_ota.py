"""Build a standard Zigbee OTA image from an ESP application (not merged flash)."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAGIC = 0x0BEEF11E
HEADER = struct.Struct('<IHHHHHIH32sI')

def metadata():
    text = (ROOT / 'firmware/HumidifierKit/version.h').read_text()
    values = {}
    for name in ('FW_FILE_VERSION', 'OTA_MANUFACTURER', 'OTA_IMAGE_TYPE', 'HW_VERSION'):
        values[name] = int(re.search(r'#define\s+' + name + r'\s+(0x[0-9A-Fa-f]+|\d+)', text)[1], 0)
    values['version'] = re.search(r'#define FW_VERSION "([^"]+)"', text)[1]
    return values

def package(app, meta):
    if len(app) < 24 or app[0] != 0xE9:
        raise ValueError('Expected ESP application image')
    # ESP image extended header chip ID: ESP32-H2 = 16.
    if struct.unpack_from('<H', app, 12)[0] != 16:
        raise ValueError('Application is not for ESP32-H2')
    if len(app) > 0x140000:
        raise ValueError('Application exceeds OTA partition')
    # HW version range included in the optional OTA header.
    size = 60 + 6 + len(app)
    header = HEADER.pack(MAGIC, 0x0100, 60, 4, meta['OTA_MANUFACTURER'],
                         meta['OTA_IMAGE_TYPE'], meta['FW_FILE_VERSION'], 2,
                         b'HumidifierKit-H2'.ljust(32, b'\0'), size)
    return header + struct.pack('<HHHI', meta['HW_VERSION'], meta['HW_VERSION'], 0, len(app)) + app

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', type=Path, default=ROOT / 'build/HumidifierKit.ino.bin')
    parser.add_argument('--out', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    meta = metadata()
    image = package(args.app.read_bytes(), meta)
    args.out.mkdir(parents=True, exist_ok=True)
    name = f"HumidifierKit-H2-{meta['version']}.ota"
    (args.out / name).write_bytes(image)
    entry = dict(fileVersion=meta['FW_FILE_VERSION'], fileSize=len(image),
                 manufacturerCode=meta['OTA_MANUFACTURER'], imageType=meta['OTA_IMAGE_TYPE'],
                 modelId='HumidifierKit-H2', manufacturerName='Orest',
                 sha512=hashlib.sha512(image).hexdigest(),
                 url=f'https://github.com/OrestHudyma/HumidifierKit/releases/download/v{meta["version"]}/{name}')
    (args.out / 'index.json').write_text(json.dumps([entry], indent=2) + '\n')
    print(f'{name}: {len(image)} bytes, fileVersion={meta["FW_FILE_VERSION"]}')

if __name__ == '__main__':
    main()

# Bench validation — 2026-11-15

Board detected via USB Serial/JTAG on COM3: ESP32-H2FH4 rev 0.1, 4 MB flash.
IEEE address: `0x744dbdfffe65d084`.

- Firmware compiled with Arduino-ESP32 3.3.7, ZCZR library, ROUTER role.
- Full original flash backed up before first write (SHA256:
  `3501DF7E63899C8862F3767B654D68CA4BCBCE86F2FA6A3E71C38F3DCF544543`).
- esptool verified written firmware hash.
- USB diagnostics exercised all eight relay output combinations, verified by GPIO
  readback; outputs restored OFF.
- Zigbee2MQTT 2.14.3-1 loaded the external converter and successfully interviewed
  and configured the device. Network channel 25.
- Device renamed `humidifierkit`; MQTT base topic `zigbee2mqtt`.
- Commands over MQTT/Zigbee produced GPIO states 0,1,1 and 1,0,0, independently
  checked over USB. All outputs then returned OFF.
- Tank input with internal pull-up and no wire reported `water_empty_tank=false`.
- Home Assistant MQTT discovery configs published for power, backlight,
  max_power, water_empty_tank, linkquality and last_seen.
- Network retained after application-only firmware update/reboot.
- Permit join closed after pairing.

Not yet physically tested: tank input closed to GND and released; physical
BOOT-button timing/factory reset; actual relay electronics; power-up transients;
coordinator loss/recovery and long-duration stability. Native upstream inclusion
is pending. The installed converter is external.

Logs and build artifacts are under `work/` and `build/` (gitignored).

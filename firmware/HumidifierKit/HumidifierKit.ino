#include <Arduino.h>
#include <Zigbee.h>
#include <driver/gpio.h>
#include "board_config.h"
#include "version.h"
#include <atomic>
#include <esp_ota_ops.h>

#ifndef ZIGBEE_MODE_ZCZR
#error Select Zigbee ZCZR mode (the application uses ROUTER role).
#endif

ZigbeePowerOutlet powerOutlet(1), backlightOutlet(2), maxPowerOutlet(3);
ZigbeeBinary waterSensor(4);
bool waterRaw = false, waterStable = false;
uint32_t waterChangedAt = 0;
bool buttonRaw = false, buttonStable = false;
uint32_t buttonChangedAt = 0, buttonPressedAt = 0;
bool zbReady = false;
std::atomic<bool> otaRunning{false};
void onOTA(bool active) {
  otaRunning.store(active);
  Serial.printf("OTA active=%d\n", active);
}

void writeRelay(uint8_t pin, bool on) {
  digitalWrite(pin, on == RELAY_ACTIVE_HIGH ? HIGH : LOW);
}
void onPower(bool on) { writeRelay(POWER_PIN, on); }
void onBacklight(bool on) { writeRelay(BACKLIGHT_PIN, on); }
void onMaxPower(bool on) { writeRelay(MAX_POWER_PIN, on); }
bool readWater() { return (digitalRead(WATER_EMPTY_PIN) == HIGH) == WATER_EMPTY_ACTIVE_HIGH; }

void printStatus() {
  Serial.printf("STATUS fw=" FW_VERSION " started=%d joined=%d power=%d backlight=%d max_power=%d water_empty=%d gpio=%d,%d,%d\n",
    zbReady, Zigbee.connected(), powerOutlet.getPowerOutletState(), backlightOutlet.getPowerOutletState(),
    maxPowerOutlet.getPowerOutletState(), waterStable,
    digitalRead(POWER_PIN), digitalRead(BACKLIGHT_PIN), digitalRead(MAX_POWER_PIN));
  Serial.printf("OTA version=0x%08lx active=%d partition=%s\n", (unsigned long)FW_FILE_VERSION,
    otaRunning.load(), esp_ota_get_running_partition()->label);
}

void requestJoin() {
  if (!zbReady || Zigbee.connected()) {
    Serial.println("JOIN skipped: stack not ready or already joined");
    return;
  }
  esp_zb_lock_acquire(portMAX_DELAY);
  esp_err_t result = esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
  esp_zb_lock_release();
  Serial.printf("JOIN result=%d\n", result);
}

// USB diagnostics. No simulated tank state; the sensor always reads the GPIO.
void handleSerial() {
  static char line[64];
  static size_t used = 0;
  static bool overflow = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (used < sizeof(line) - 1) line[used++] = c;
      else overflow = true;
      continue;
    }
    line[used] = 0;
    if (overflow) Serial.println("ERROR line too long");
    else if (!strcmp(line, "status")) printStatus();
    else if (!strcmp(line, "join")) requestJoin();
    else if (zbReady) {
      ZigbeePowerOutlet *outlet = nullptr;
      if (!strncmp(line, "power ", 6)) outlet = &powerOutlet;
      if (!strncmp(line, "backlight ", 10)) outlet = &backlightOutlet;
      if (!strncmp(line, "max_power ", 10)) outlet = &maxPowerOutlet;
      const char *value = strchr(line, ' ');
      if (outlet && value && (!strcmp(value + 1, "on") || !strcmp(value + 1, "off"))) {
        bool ok = outlet->setState(!strcmp(value + 1, "on"));
        Serial.println(ok ? "OK" : "ERROR attribute update");
        printStatus();
      } else Serial.println("ERROR command");
    } else Serial.println("ERROR Zigbee not ready");
    used = 0;
    overflow = false;
  }
}

void setup() {
  for (uint8_t pin : {POWER_PIN, BACKLIGHT_PIN, MAX_POWER_PIN}) {
    // Preload the output latch before enabling the driver; Arduino digitalWrite
    // cannot be used until pinMode has attached the GPIO peripheral.
    gpio_set_level(static_cast<gpio_num_t>(pin), RELAY_ACTIVE_HIGH ? 0 : 1);
    pinMode(pin, OUTPUT);
    writeRelay(pin, false);
  }
  pinMode(WATER_EMPTY_PIN, INPUT_PULLUP);
  pinMode(PAIR_BUTTON_PIN, INPUT_PULLUP);
  waterRaw = waterStable = readWater();
  Serial.begin(115200);
  // No wait for USB host: this device also boots on a standalone power supply.
  powerOutlet.onPowerOutletChange(onPower);
  backlightOutlet.onPowerOutletChange(onBacklight);
  maxPowerOutlet.onPowerOutletChange(onMaxPower);
  if (!powerOutlet.addOTAClient(FW_FILE_VERSION, FW_FILE_VERSION, HW_VERSION,
                                OTA_MANUFACTURER, OTA_IMAGE_TYPE, 50)) {
    Serial.println("FATAL: OTA client registration failed");
    while (true) delay(1000);
  }
  powerOutlet.onOTAStateChange(onOTA);
  waterSensor.addBinaryInput();
  waterSensor.setBinaryInputDescription("Water tank empty");
  waterSensor.setBinaryInputApplication(BINARY_INPUT_APPLICATION_TYPE_HVAC_OTHER);
  ZigbeeEP *endpoints[] = {&powerOutlet, &backlightOutlet, &maxPowerOutlet, &waterSensor};
  for (auto *ep : endpoints) {
    ep->setManufacturerAndModel("Orest", "HumidifierKit-H2");
    ep->setVersion(2);
    ep->setHardwareVersion(HW_VERSION);
    Zigbee.addEndpoint(ep);
  }
  Zigbee.setTimeout(10000);
  zbReady = Zigbee.begin(ZIGBEE_ROUTER);
  if (zbReady) {
    powerOutlet.setState(false);
    backlightOutlet.setState(false);
    maxPowerOutlet.setState(false);
    waterSensor.setBinaryInput(waterStable);
  }
  printStatus();
}

void loop() {
  const uint32_t now = millis();
  bool raw = readWater();
  if (raw != waterRaw) { waterRaw = raw; waterChangedAt = now; }
  if (waterStable != waterRaw && now - waterChangedAt >= DEBOUNCE_MS) {
    waterStable = waterRaw;
    if (zbReady) {
      waterSensor.setBinaryInput(waterStable);
      if (Zigbee.connected()) waterSensor.reportBinaryInput();
    }
    Serial.printf("WATER empty=%d\n", waterStable);
  }
  bool pressed = digitalRead(PAIR_BUTTON_PIN) == LOW;
  if (pressed != buttonRaw) { buttonRaw = pressed; buttonChangedAt = now; }
  if (buttonStable != buttonRaw && now - buttonChangedAt >= DEBOUNCE_MS) {
    buttonStable = buttonRaw;
    if (buttonStable) buttonPressedAt = now;
    else {
      uint32_t duration = now - buttonPressedAt;
      // Act on release so the BOOT pin is high when the device restarts.
      if (duration >= 10000 && zbReady && !otaRunning.load()) {
        Serial.println("FACTORY RESET");
        Serial.flush();
        Zigbee.factoryReset();
      } else if (duration >= 3000) requestJoin();
    }
  }
  static bool wasJoined = false;
  static bool otaQueryStarted = false;
  static uint32_t lastReport = 0, lastStatus = 0;
  bool joined = Zigbee.connected();
  if (zbReady && joined && !otaQueryStarted) {
    powerOutlet.requestOTAUpdate();
    otaQueryStarted = true;
  }
  if (zbReady && joined && (!wasJoined || now - lastReport >= 60000)) {
    waterSensor.reportBinaryInput();
    lastReport = now;
  }
  wasJoined = joined;
  handleSerial();
  if (now - lastStatus >= 10000) { printStatus(); lastStatus = now; }
  delay(5);
}


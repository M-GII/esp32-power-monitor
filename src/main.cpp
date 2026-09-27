#include <Arduino.h>
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "secrets.h"

const int VOLTAGE_PIN = 35;
const int CURRENT_PIN = 34;
const int MOSFET_PIN  = 26;

const float R1 = 20000.0;
const float R2 = 10000.0;


// ACS712 5A sensitivity
const float ACS_SENSITIVITY = 0.185; // V/A

// Measured zero-current voltage
const float ACS_ZERO_VOLTAGE = 2.540;


// Current >= 0.30A -> WARNING
const float WARNING_CURRENT = 0.30;

// Current >= 0.40A -> FAULT
//~10 ohm load should trigger this
const float FAULT_CURRENT = 0.40;

const int NUM_SAMPLES = 64;

enum SystemState {
  NORMAL = 0,
  WARNING = 1,
  FAULT = 2
};

SystemState state = NORMAL;

bool faultLatched = false;

ModbusIP mb;

// Holding register map:
//
// HR100 = Voltage x 1000
// HR101 = Current x 1000
// HR102 = Power   x 1000
// HR103 = State
//         0 = NORMAL
//         1 = WARNING
//         2 = FAULT
//
// HR104 = Load status
//         0 = OFF
//         1 = ON
//

const uint16_t REG_VOLTAGE = 100;
const uint16_t REG_CURRENT = 101;
const uint16_t REG_POWER   = 102;
const uint16_t REG_STATE   = 103;
const uint16_t REG_LOAD    = 104;

float readVoltage() {

  uint32_t totalMv = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {

    totalMv += analogReadMilliVolts(VOLTAGE_PIN);

    // Keep Modbus responsive while sampling
    mb.task();

    delay(2);
  }

  float adcMilliVolts =
      totalMv / (float)NUM_SAMPLES;

  float adcVoltage =
      adcMilliVolts / 1000.0;

  float supplyVoltage =
      adcVoltage * ((R1 + R2) / R2);

  return supplyVoltage;
}

float readCurrent() {

  uint32_t totalMv = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {

    totalMv += analogReadMilliVolts(CURRENT_PIN);

    mb.task();

    delay(2);
  }

  float sensorMilliVolts =
      totalMv / (float)NUM_SAMPLES;

  float sensorVoltage =
      sensorMilliVolts / 1000.0;

  float current =
      (sensorVoltage - ACS_ZERO_VOLTAGE)
      / ACS_SENSITIVITY;

  // Remove small ACS712 zero-current noise
  if (current > -0.03 && current < 0.03) {
    current = 0.0;
  }

  return current;
}

void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}


void setup() {

  Serial.begin(115200);

  analogReadResolution(12);

  analogSetPinAttenuation(
      VOLTAGE_PIN,
      ADC_11db
  );

  analogSetPinAttenuation(
      CURRENT_PIN,
      ADC_11db
  );
  pinMode(MOSFET_PIN, OUTPUT);

  // Start with load enabled
  digitalWrite(MOSFET_PIN, HIGH);

  connectWiFi();
  mb.server();

  mb.addHreg(REG_VOLTAGE, 0);
  mb.addHreg(REG_CURRENT, 0);
  mb.addHreg(REG_POWER,   0);
  mb.addHreg(REG_STATE,   0);
  mb.addHreg(REG_LOAD,    1);

  Serial.println();
  Serial.println("Modbus TCP server started");

  Serial.println("Holding Registers:");
  Serial.println("HR100 = Voltage x1000");
  Serial.println("HR101 = Current x1000");
  Serial.println("HR102 = Power x1000");
  Serial.println("HR103 = State");
  Serial.println("HR104 = Load status");

  Serial.println();
  Serial.println("ESP32 Power Protection System");
  Serial.println();
}

void loop() {

  mb.task();

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi disconnected. Reconnecting...");

    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {

      mb.task();
      delay(500);
      Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi reconnected");
    Serial.println(WiFi.localIP());
  }

  float voltage = readVoltage();
  float current = readCurrent();
  if (current < 0.0) {
    current = 0.0;
  }

  float power = voltage * current;


  if (!faultLatched &&
      current >= FAULT_CURRENT) {

    faultLatched = true;
  }

  if (faultLatched) {

    state = FAULT;

  } else if (current >= WARNING_CURRENT) {

    state = WARNING;

  } else {

    state = NORMAL;
  }

  bool loadEnabled;

  if (state == FAULT) {

    digitalWrite(MOSFET_PIN, LOW);

    loadEnabled = false;

  } else {

    digitalWrite(MOSFET_PIN, HIGH);

    loadEnabled = true;
  }


  mb.Hreg(
      REG_VOLTAGE,
      (uint16_t)(voltage * 1000.0)
  );

  mb.Hreg(
      REG_CURRENT,
      (uint16_t)(current * 1000.0)
  );

  mb.Hreg(
      REG_POWER,
      (uint16_t)(power * 1000.0)
  );

  mb.Hreg(
      REG_STATE,
      (uint16_t)state
  );

  mb.Hreg(
      REG_LOAD,
      loadEnabled ? 1 : 0
  );


  Serial.println(
      "========== TELEMETRY =========="
  );

  Serial.print("Voltage:      ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  Serial.print("Current:      ");
  Serial.print(current, 3);
  Serial.println(" A");

  Serial.print("Power:        ");
  Serial.print(power, 3);
  Serial.println(" W");

  Serial.print("State:        ");

  if (state == NORMAL) {

    Serial.println("NORMAL");

  } else if (state == WARNING) {

    Serial.println("WARNING");

  } else {

    Serial.println("FAULT");
  }

  Serial.print("Load:         ");

  if (loadEnabled) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  Serial.println();
  Serial.println("Modbus Registers:");

  Serial.print("HR100 Voltage: ");
  Serial.println(
      mb.Hreg(REG_VOLTAGE)
  );

  Serial.print("HR101 Current: ");
  Serial.println(
      mb.Hreg(REG_CURRENT)
  );

  Serial.print("HR102 Power:   ");
  Serial.println(
      mb.Hreg(REG_POWER)
  );

  Serial.print("HR103 State:   ");
  Serial.println(
      mb.Hreg(REG_STATE)
  );

  Serial.print("HR104 Load:    ");
  Serial.println(
      mb.Hreg(REG_LOAD)
  );

  Serial.println(
      "==============================="
  );

  Serial.println();

  // Rather than blocking for one full second,
  // keep servicing Modbus during the wait.
  unsigned long start = millis();

  while (millis() - start < 1000) {

    mb.task();

    delay(10);
  }
}
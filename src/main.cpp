#include <Arduino.h>

const int VOLTAGE_PIN = 35;
const int CURRENT_PIN = 34;
const int MOSFET_PIN  = 26;

const float R1 = 20000.0;
const float R2 = 10000.0;

const float ACS_SENSITIVITY = 0.185;
const float ACS_ZERO_VOLTAGE = 2.540;

const float WARNING_CURRENT = 0.35; // A
const float FAULT_CURRENT   = 0.40; // A

const int NUM_SAMPLES = 64;

enum SystemState {
  NORMAL,
  WARNING,
  FAULT
};

SystemState state = NORMAL;
bool faultLatched = false;

float readVoltage() {
  uint32_t totalMv = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    totalMv += analogReadMilliVolts(VOLTAGE_PIN);
    delay(2);
  }

  float adcVoltage =
      (totalMv / (float)NUM_SAMPLES) / 1000.0;

  return adcVoltage * ((R1 + R2) / R2);
}

float readCurrent() {
  uint32_t totalMv = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    totalMv += analogReadMilliVolts(CURRENT_PIN);
    delay(2);
  }

  float sensorVoltage =
      (totalMv / (float)NUM_SAMPLES) / 1000.0;

  return (sensorVoltage - ACS_ZERO_VOLTAGE)
         / ACS_SENSITIVITY;
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN, ADC_11db);

  pinMode(MOSFET_PIN, OUTPUT);

  // Start load ON
  digitalWrite(MOSFET_PIN, HIGH);

  Serial.println("ESP32 Power Protection System");
}

void loop() {
  float voltage = readVoltage();
  float current = readCurrent();

  // Ignore tiny ACS712 noise around zero
  if (current > -0.03 && current < 0.03) {
    current = 0.0;
  }

  // Fault detection
  if (!faultLatched && current >= FAULT_CURRENT) {
    faultLatched = true;
  }

  // State selection
  if (faultLatched) {
    state = FAULT;
  }
  else if (current >= WARNING_CURRENT) {
    state = WARNING;
  }
  else {
    state = NORMAL;
  }

  // Protection output
  if (state == FAULT) {
    digitalWrite(MOSFET_PIN, LOW);
  }
  else {
    digitalWrite(MOSFET_PIN, HIGH);
  }

  float power = voltage * current;

  Serial.println("========== TELEMETRY ==========");

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
  }
  else if (state == WARNING) {
    Serial.println("WARNING");
  }
  else {
    Serial.println("FAULT");
  }

  Serial.print("Load:         ");
  Serial.println(state == FAULT ? "OFF" : "ON");

  Serial.println("===============================");
  Serial.println();

  delay(1000);
}
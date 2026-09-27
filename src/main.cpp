#include <Arduino.h>

const int VOLTAGE_PIN = 35;
const int CURRENT_PIN = 34;

const float R1 = 20000.0;
const float R2 = 10000.0;

const float ACS_SENSITIVITY = 0.185; // 5A ACS712
float acsZeroVoltage = 2.54;

const int NUM_SAMPLES = 64;

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN, ADC_11db);

  Serial.println("Voltage + Current Test");
}

void loop() {
  uint32_t voltageTotalMv = 0;
  uint32_t currentTotalMv = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    voltageTotalMv += analogReadMilliVolts(VOLTAGE_PIN);
    currentTotalMv += analogReadMilliVolts(CURRENT_PIN);
    delay(2);
  }

  float adcVoltage =
      (voltageTotalMv / (float)NUM_SAMPLES) / 1000.0;

  float supplyVoltage =
      adcVoltage * ((R1 + R2) / R2);

  float acsVoltage =
      (currentTotalMv / (float)NUM_SAMPLES) / 1000.0;

  float current =
      (acsVoltage - acsZeroVoltage) / ACS_SENSITIVITY;

  Serial.println("--------- TELEMETRY ---------");

  Serial.print("Supply Voltage: ");
  Serial.print(supplyVoltage, 3);
  Serial.println(" V");

  Serial.print("ACS712 OUT:     ");
  Serial.print(acsVoltage, 3);
  Serial.println(" V");

  Serial.print("Current:        ");
  Serial.print(current, 3);
  Serial.println(" A");

  Serial.println("-----------------------------");
  Serial.println();

  delay(1000);
}
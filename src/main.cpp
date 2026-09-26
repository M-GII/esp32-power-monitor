#include <Arduino.h>

const int VOLTAGE_PIN = 35;

// Voltage divider:
// 5V -> 10k -> 10k -> D35 -> 10k -> GND
const float R1 = 20000.0;  // top resistance
const float R2 = 10000.0;  // bottom resistance

const int NUM_SAMPLES = 64;

void setup() {
  Serial.begin(115200);

  pinMode(VOLTAGE_PIN, INPUT);

  // 12-bit ADC: 0-4095
  analogReadResolution(12);

  // Needed because divider output is around 1.7V
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" ESP32 VOLTAGE MONITOR");
  Serial.println("==============================");
}

void loop() {

  uint32_t totalMilliVolts = 0;

  // Average multiple readings for a steadier value
  for (int i = 0; i < NUM_SAMPLES; i++) {
    totalMilliVolts += analogReadMilliVolts(VOLTAGE_PIN);
    delay(2);
  }

  float adcMilliVolts =
      totalMilliVolts / (float)NUM_SAMPLES;

  float adcVoltage =
      adcMilliVolts / 1000.0;

  // Divider ratio:
  // (20k + 10k) / 10k = 3
  float supplyVoltage =
      adcVoltage * ((R1 + R2) / R2);

  Serial.println("---------- VOLTAGE ----------");

  Serial.print("ADC Pin Voltage: ");
  Serial.print(adcVoltage, 3);
  Serial.println(" V");

  Serial.print("Supply Voltage:  ");
  Serial.print(supplyVoltage, 3);
  Serial.println(" V");

  Serial.println("-----------------------------");
  Serial.println();

  delay(1000);
}
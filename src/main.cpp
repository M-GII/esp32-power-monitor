#include <Arduino.h>
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include "secrets.h"

ModbusIP mb;

const int TEST_REGISTER = 100;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start Modbus TCP server
  mb.server();

  // Holding register 100
  mb.addHreg(TEST_REGISTER, 1234);

  Serial.println("Modbus TCP server started");
  Serial.println("Register 100 = 1234");
}

void loop() {
  mb.task();
  delay(10);
}
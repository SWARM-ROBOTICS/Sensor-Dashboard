#include <Arduino.h>
#include <DFRobot_Geiger.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <BME280I2C.h>
#include <ICM_20948.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <esp_now.h>
#include <WiFi.h>
#include "SparkFun_STTS22H.h"
#include <Adafruit_LPS2X.h>

#define SERIAL_BAUD 115200

// Define Geiger counter
const int geigerPin = D3;
DFRobot_Geiger geiger(geigerPin);

// Define sensor objects
Adafruit_SHT31 sht31 = Adafruit_SHT31();
BME280I2C bme;
ICM_20948_I2C myICM_1;
Adafruit_BME680 bme680;
SparkFun_STTS22H mySTTS;
Adafruit_LPS22 lps;

long previousMillis = 0;
long currentMillis = 0;
long dt = 0;

double data[16];
float sensorData[17];

typedef struct struct_message {
  float sensorValues[17];
} struct_message;

struct_message dataToSend;

uint8_t broadcastAddress[] = { 0xF0, 0x9E, 0x9E, 0x3B, 0xF5, 0xFC };
esp_now_peer_info_t peerInfo;

// Callback for data sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);

  Wire.begin(D4, D5);
  Wire.setClock(400000); // Set I2C clock speed

  // Initialize BME680
  if (!bme680.begin(0x77)) {
    Serial.println("Couldn't find the BME680 sensor");
    while (1);
  }

  // Set ESP32 to station mode and initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Add peer for ESP-NOW
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Register callback for data sent
  esp_now_register_send_cb(OnDataSent);

  // Initialize Geiger counter
  geiger.start();
}

void loop() {
  currentMillis = millis();
  dt = currentMillis - previousMillis;

  //Read BME680
  if (bme680.begin(0x77)) {
    if (bme680.performReading()) {
      sensorData[0]=bme680.temperature;
      sensorData[1]=bme680.humidity;
      sensorData[2]=bme680.pressure / 100.0;
      sensorData[3]=bme680.gas_resistance / 1000.0;
    }
  }

  // Send all sensor data via ESP-NOW
  for (int i = 0; i < 4; i++) {
    dataToSend.sensorValues[i] = sensorData[i];
  }
  esp_now_send(broadcastAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));

  // Print the data for debugging
  Serial.print("Sensor Data: [");
  for (int i = 0; i < 4; i++) {
    Serial.print(sensorData[i], 2);
    if (i < 3) {
      Serial.print(", ");
    }
  }
  Serial.println("]");

  delay(100); // Delay between sensor readings
  previousMillis = currentMillis;
  Serial.println(dt);
}

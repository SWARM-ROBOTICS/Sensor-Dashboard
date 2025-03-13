#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>

#define SERIAL_BAUD 115200

WebServer server(80);

long previousMillis = 0;
long currentMillis = 0;
long dt = 0;

typedef struct struct_message {
  float sensorValues[17];
} struct_message;

struct_message receivedData;

bool dataReceived = false;

String getHTMLPage() {
  String html = R"=====( 
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Sensor Dashboard</title>
  <style>
    body {
      font-family: 'Arial', sans-serif;
      background-color: #1c1c1e;
      color: #f4f4f4;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      flex-direction: column;
    }
    h1 {
      color: #ffd700;
      margin-bottom: 20px;
      font-size: 2rem;
      text-transform: uppercase;
      letter-spacing: 2px;
    }
    .dashboard {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 20px;
      width: 90%;
      max-width: 900px;
    }
    .tile {
      background-color: #2c2c2e;
      border: 2px solid #ffd700;
      border-radius: 15px;
      box-shadow: 0 6px 12px rgba(0, 0, 0, 0.4);
      padding: 20px;
      text-align: center;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    .tile:hover {
      transform: translateY(-5px);
      box-shadow: 0 10px 20px rgba(0, 0, 0, 0.6);
    }
    .tile h2 {
      margin: 0;
      font-size: 1.5rem;
      color: #ffd700;
    }
    .tile p {
      font-size: 2rem;
      font-weight: bold;
      color: #f4f4f4;
      margin: 10px 0 0;
    }
    .tile span {
      font-size: 1rem;
      color: #bfbfbf;
    }
  </style>
  <script>
    function fetchData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('sensor0').innerHTML = data.sensorValues[0].toFixed(2) + " <span>ppm</span>";
          document.getElementById('sensor1').innerHTML = data.sensorValues[1].toFixed(2) + " <span>ppm</span>";
          document.getElementById('sensor2').innerHTML = data.sensorValues[2].toFixed(2) + " <span>ppm</span>";
          document.getElementById('sensor3.0').innerHTML = data.sensorValues[3].toFixed(2) + " <span>°C</span>";
          document.getElementById('sensor3.1').innerHTML = data.sensorValues[4].toFixed(2) + " <span>%</span>";
          document.getElementById('sensor3.2').innerHTML = data.sensorValues[5].toFixed(2) + " <span>hPa</span>";
          document.getElementById('sensor4').innerHTML = data.sensorValues[6].toFixed(2) + " <span>ppm</span>";
          document.getElementById('sensor5').innerHTML = data.sensorValues[7].toFixed(2) + " <span>µg/m³</span>";
        });
    }
    setInterval(fetchData, 1000);
    window.onload = fetchData;
  </script>
</head>
<body>
  <h1>ESP32 Sensor Dashboard</h1>
  <div class="dashboard">
    <div class="tile">
      <h2>MQ4</h2>
      <p id="sensor0">0.00 <span>ppm</span></p>
    </div>
    <div class="tile">
      <h2>MQ7</h2>
      <p id="sensor1">0.00 <span>ppm</span></p>
    </div>
    <div class="tile">
      <h2>MQ8</h2>
      <p id="sensor2">0.00 <span>ppm</span></p>
    </div>
    <div class="tile">
      <h2>BME680</h2>
      <p id="sensor3.0">0.00 <span>°C</span></p>
      <p id="sensor3.1">0.00 <span>%</span></p>
      <p id="sensor3.2">0.00 <span>hPa</span></p>
    </div>
    <div class="tile">
      <h2>MQ137</h2>
      <p id="sensor4">0.00 <span>ppm</span></p>
    </div>
    <div class="tile">
      <h2>Geiger Counter</h2>
      <p id="sensor5">0.00 <span>µg/m³</span></p>
    </div>
  </div>
</body>
</html>
)=====";
  return html;
}

void handleData() {
  String json = "{";
  json += "\"sensorValues\":[";
  for (int i = 0; i < 7; i++) {
    json += String(receivedData.sensorValues[i], 2);
    if (i < 6) {
      json += ",";
    }
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void OnDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  dataReceived = true;
}

void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("SWARM ROBOTICS");
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  if (esp_now_init() != ESP_OK) {
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  server.handleClient();
  if (dataReceived) {
    currentMillis = millis();
    dt = currentMillis - previousMillis;
    Serial.print("Sensor Data: [");
    for (int i = 0; i < 7; i++) {
      Serial.print(receivedData.sensorValues[i], 2);
      if (i < 6) {
        Serial.print(", ");
      }
    }
    Serial.println("]");
    dataReceived = false;
    previousMillis = currentMillis;
    Serial.println(dt);
  }
}

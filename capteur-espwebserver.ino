#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>

#define WIFI_SSID     "iPhone de Mathurin"
#define WIFI_PASSWORD "cocoviens"

#define API_URL "https://mleclere-github-io.onrender.com/sensor"

#define DHT_PIN       26
#define DHT_TYPE      DHT11

const int SOL_POWER  = 25;  
const int GAS_PIN    = 35;  
const int LIGHT_PIN  = 32;  

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

int airHumidity  = 0;
int temperature  = 0;
int soilHumidity = 0;
int gas          = 0;
int light        = 0;

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 10000; 

void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Erreur lecture DHT11 !");
    return;
  }

  airHumidity = (int)h;
  temperature = (int)t;

  Serial.print("Température : ");
  Serial.print(temperature);
  Serial.print(" °C  |  Humidité air : ");
  Serial.print(airHumidity);
  Serial.println(" %");
}

void readSensors() {
  delay(100);
  int rawSoil = analogRead(SOL_POWER);
  soilHumidity = constrain(map(rawSoil, 0, 4095, 0, 100), 0, 100);

  int rawGas = analogRead(GAS_PIN);
  gas = constrain(map(rawGas, 0, 4095, 0, 100), 0, 100);

  int rawLight = analogRead(LIGHT_PIN);
  light = constrain(map(rawLight, 0, 4095, 0, 100), 0, 100);

  Serial.print("Sol : "); Serial.print(soilHumidity); Serial.print(" %");
  Serial.print("  |  Gaz : "); Serial.print(gas); Serial.print(" %");
  Serial.print("  |  Lumière : "); Serial.print(light); Serial.println(" %");
}

void sendToAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi déconnecté, envoi API annulé.");
    return;
  }

  String url = String(API_URL) + "?";
  url += "airHumidity="  + String(airHumidity);
  url += "&soilHumidity=" + String(soilHumidity);
  url += "&temperature=" + String(temperature);
  url += "&gas="         + String(gas);
  url += "&light="       + String(light);

  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  Serial.print("API Response : ");
  Serial.println(httpResponseCode);
  http.end();
}

void pageHtml() {
  String response = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 - Capteurs</title>
      <meta name='viewport' content='width=device-width, initial-scale=1'>
      <meta http-equiv='refresh' content='5'>
    </head>
    <body>
      <h1>ESP32 - Capteurs</h1>
      <p>🌡️ Température : )" + String(temperature) + R"( °C</p>
      <p>💧 Humidité air : )" + String(airHumidity) + R"( %</p>
      <p>🌱 Humidité sol : )" + String(soilHumidity) + R"( %</p>
      <p>💨 Gaz : )" + String(gas) + R"( %</p>
      <p>☀️ Luminosité : )" + String(light) + R"( %</p>
    </body>
    </html>
  )";
  server.send(200, "text/html", response);
}

void handlePost() {
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (!error) {
    server.send(200, "text/plain", "JSON reçu !");
  } else {
    server.send(400, "text/plain", "Erreur JSON");
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(SOL_POWER, OUTPUT);
  digitalWrite(SOL_POWER, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connecté !");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());

  server.on("/", pageHtml);
  server.on("/data", HTTP_POST, handlePost);
  server.begin();
}

void loop() {
  server.handleClient();

  readDHT();
  readSensors();

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendToAPI();
    lastSendTime = millis();
  }

  delay(2000);
}
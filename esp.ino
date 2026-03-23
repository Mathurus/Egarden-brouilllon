#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#define WIFI_SSID     "iPhone de Mathurin"
#define WIFI_PASSWORD "cocoviens"
#define API_URL       "https://mleclere-github-io.onrender.com/sensor"
#define SEND_INTERVAL 10000 

WebServer server(80);

int temperature  = 22;
int airHumidity  = 32;
int soilHumidity = 52;
int gas          = 33;
int light        = 66;

unsigned long lastSendTime = 0;

void sendToAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi déconnecté, skip envoi API");
        return;
    }

    String url = String(API_URL) + "?";
    url += "airHumidity="   + String(airHumidity);
    url += "&soilHumidity=" + String(soilHumidity);
    url += "&temperature="  + String(temperature);
    url += "&gas="          + String(gas);
    url += "&light="        + String(light);

    WiFiClientSecure client;
    client.setInsecure(); 

    HTTPClient http;
    http.begin(client, url);

    int httpCode = http.GET();
    Serial.print("API Response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.print("Réponse : ");
        Serial.println(payload);
    } else {
        Serial.print("Erreur HTTP : ");
        Serial.println(http.errorToString(httpCode));
    }

    http.end();
}

void pageHtml() {
    String response = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>ESP32 Server</title>
            <meta name='viewport' content='width=device-width, initial-scale=1'>
        </head>
        <body>
            <h1>ESP32 Server</h1>
            <p>Température : )" + String(temperature) + R"(°C</p>
            <p>Humidité air : )" + String(airHumidity) + R"(%</p>
            <p>Humidité sol : )" + String(soilHumidity) + R"(%</p>
            <p>Gaz : )" + String(gas) + R"(</p>
            <p>Lumière : )" + String(light) + R"(</p>
        </body>
        </html>
    )";
    server.send(200, "text/html", response);
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connexion WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println(" Connecté !");
    Serial.print("IP : ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, pageHtml);
    server.begin();
    Serial.println("Serveur HTTP démarré");

    sendToAPI();
    lastSendTime = millis();
}

void loop() {
    server.handleClient();

    if (millis() - lastSendTime >= SEND_INTERVAL) {
        sendToAPI();
        lastSendTime = millis();
    }

    delay(2);
}

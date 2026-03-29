#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "iPhone de Mathurin"
#define WIFI_PASSWORD "cocoviens"

#define DHT_PIN  26
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

WebServer server(80);

const int capteur_humi = 34; 
const int dht_pin = 26;
const int lumunosite_pin = 35;
int airHumidity = 0;
int temperature = 0;
int niveau = 0;
int gas = 0;
int light = 0;
int val_humi_sol = 0;

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

void pagehhtml() {
    String response = R"(
        <!DOCTYPE html>
        <html>
        <head>
        <title>Web Server</title>
        <meta name='viewport' content='width=device-width, initial-scale=1'>
        </head>
        <body>
            <h1>ESP32 Server</h1>
            <p>Temperrature: )" + String(temperature) + R"(</p>
            <p>Humidite air : )" + String(airHumidity) + R"(</p>
            <p>Humidite sol : )" + String(niveau) + R"(</p>
            <p>Gaz : )" + String(gas) + R"(</p> 
            <p>Lumunosite : )" + String(light) + R"(</p>
        </body>
        </html>
    )";

    server.send(200, "text/html", response);
}

void readDHT() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println(" | Erreur lecture DHT11 !");
        return;
    }

    airHumidity = (int)h;
    temperature = (int)t;

    Serial.print(" | Température : ");
    Serial.print(temperature);
    Serial.print("  °C  |  Humidité air : ");
    Serial.print(airHumidity);
    Serial.println(" %");
}

void setup() {
    Serial.begin(9600);
    pinMode(capteur_humi, OUTPUT);
    digitalWrite(capteur_humi, LOW);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi ");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }

    Serial.println(" Connected!");
    Serial.println(WiFi.localIP());

    server.on("/", pagehhtml);
    server.begin();

    String url = "https://mleclere-github-io.onrender.com/sensor?";
    url += "airHumidity=" + String(airHumidity);
    url += "&soilHumidity=" + String(niveau);
    url += "&temperature=" + String(temperature);
    url += "&gas=" + String(gas);
    url += "&light=" + String(light);

    HTTPClient http;
    http.begin(url);

    int httpResponseCode = http.GET();
    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
    server.on("/data", HTTP_POST, handlePost);
}

void loop() {
    readDHT();
    val_humi_sol = analogRead(capteur_humi); // Sans "int" devant
    niveau = map(val_humi_sol, 0, 4095, 0, 100); // Sans "int" devant
    niveau = constrain(niveau, 0, 100);
 
    int valeur_light = analogRead(lumunosite_pin);
    Serial.print("Light : ")
    Serial.print(valeur_light); 
    Serial.print("Sensor value: ");
    Serial.println(niveau);
    server.handleClient();
    delay(500);
}
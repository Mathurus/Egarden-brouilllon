#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <DHT.h>

#define WIFI_SSID     "iPhone de Mathurin"
#define WIFI_PASSWORD "cocoviens"
#define API_URL       "https://mleclere-github-io.onrender.com/sensor"
#define SEND_INTERVAL 10000

// ── DHT11 ──────────────────────────────────────────────
#define DHT_PIN  15      // Broche DATA du DHT11 (à adapter selon ton câblage)
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ── LCD ────────────────────────────────────────────────
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

WebServer server(80);

unsigned long lastSendTime = 0;

// ── Broches ────────────────────────────────────────────
const int bouton1              = 8;
const int motorpin             = 9;
const int capteur_humidite     = A0;
const int capteur_luminosite   = A1;
const int capteur_gaz          = A2;
const int motorpin2            = 6;

// ── Variables globales capteurs ────────────────────────
int temperature  = 0;   // lu par le DHT11
int airHumidity  = 0;   // lu par le DHT11
int soilHumidity = 0;   // lu en analogique
int gas          = 0;   // lu en analogique
int light        = 0;   // lu en analogique

bool ancienEtatBouton = HIGH;
int  etat             = 0;

Servo monservo_1;
Servo monservo_2;

// ── Lecture DHT11 ──────────────────────────────────────
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

// ── Envoi API ──────────────────────────────────────────
void sendToAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi déconnecté, skip envoi API");
        return;
    }

    // Lecture des capteurs analogiques avant envoi
    soilHumidity = (long)analogRead(capteur_humidite)   * 100 / 876;
    gas          = (long)analogRead(capteur_gaz)         * 100 / 380;
    light        = (long)analogRead(capteur_luminosite)  * 100 / 471;

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

// ── Page Web ───────────────────────────────────────────
void pageHtml() {
    long humidite_sol             = (long)analogRead(capteur_humidite)  * 100 / 876;
    long gaz_valeur               = (long)analogRead(capteur_gaz)        * 100 / 380;
    long pourcentge_valeur_lumunosite = (long)analogRead(capteur_luminosite) * 100 / 471;

    // Température et humidité air → DHT11
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
            <p>Humidité sol : )" + String(humidite_sol) + R"(%</p>
            <p>Gaz : )" + String(gaz_valeur) + R"(</p>
            <p>Lumière : )" + String(pourcentge_valeur_lumunosite) + R"(</p>
        </body>
        </html>
    )";
    server.send(200, "text/html", response);
}

// ── Setup ──────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    dht.begin();
    Serial.println("DHT11 initialisé");

    lcd.begin(16, 2);
    monservo_1.attach(9);
    monservo_2.attach(7);
    pinMode(bouton1, INPUT_PULLUP);
    pinMode(10, OUTPUT);
    pinMode(6,  OUTPUT);
    pinMode(13, OUTPUT);

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

    readDHT();
    sendToAPI();
    lastSendTime = millis();
}

// ── Loop ───────────────────────────────────────────────
void loop() {
    // Mise à jour DHT11 à chaque tour (le DHT11 est limité à ~1 lecture/s,
    // mais l'appel est rapide ; les valeurs stables sont réutilisées partout)
    readDHT();

    humidifi();
    ouverture();
    refroidir();
    ajouter_lumunosite();
    rechauffer();
    limiter_humi_air();

    bool etatBouton = digitalRead(bouton1);
    if (etatBouton == LOW && ancienEtatBouton == HIGH) {
        etat++;
        if (etat > 4) etat = 0;
        lcd.clear();
    }

    int sensor3    = analogRead(capteur_gaz);
    long sensor50  = (long)sensor3 * 100 / 380;
    if (sensor50 >= 55) {
        afficher_gaz_secour();
    } else {
        ancienEtatBouton = etatBouton;
        switch (etat) {
            case 0: afficher_temperature();  break;
            case 1: afficher_humidite();     break;
            case 2: afficher_gaz();          break;
            case 3: afficher_luminosite();   break;
            case 4: afficher_humidite_air(); break;
        }
    }

    server.handleClient();

    if (millis() - lastSendTime >= SEND_INTERVAL) {
        readDHT();
        sendToAPI();
        lastSendTime = millis();
    }

    delay(2);
}

// ── Fonctions d'affichage LCD ──────────────────────────
void afficher_temperature() {
    // Utilise la variable globale temperature (lue par DHT11)
    lcd.setCursor(0, 0);
    lcd.print("Temperature:");
    lcd.setCursor(0, 1);
    lcd.print(temperature);
    lcd.print(" C   ");
}

void afficher_humidite() {
    delay(10);
    int  value_sol    = analogRead(capteur_humidite);
    long humidite_sol = (long)value_sol * 100 / 876;
    lcd.setCursor(0, 0);
    lcd.print("Humidite sol:");
    lcd.setCursor(0, 1);
    lcd.print(humidite_sol);
    lcd.print("     ");
}

void afficher_luminosite() {
    delay(50);
    int  valeur          = analogRead(capteur_luminosite);
    long pourcentge_valeur = (long)valeur * 100 / 471;
    lcd.setCursor(0, 0);
    lcd.print("Luminosite:");
    lcd.setCursor(0, 1);
    lcd.print(pourcentge_valeur);
    lcd.print("     ");
}

void afficher_gaz() {
    delay(50);
    int  sensor  = analogRead(capteur_gaz);
    long sensor49 = (long)sensor * 100 / 380;
    lcd.setCursor(0, 0);
    if (sensor49 >= 70) {
        lcd.print("Gaz ELEVE !");
    } else {
        lcd.print("Gaz:");
    }
    lcd.setCursor(0, 1);
    lcd.print(sensor49);
    lcd.print("     ");
}

void afficher_gaz_secour() {
    int  sensor2  = analogRead(capteur_gaz);
    long sensor48 = (long)sensor2 * 100 / 380;
    lcd.setCursor(0, 0);
    lcd.print("Gaz ELEVE :");
    lcd.setCursor(0, 1);
    lcd.print(sensor48);
    delay(50);
    lcd.clear();
}

void afficher_humidite_air() {
    // Utilise la variable globale airHumidity (lue par DHT11)
    lcd.setCursor(0, 0);
    lcd.print("Humidite air:");
    lcd.setCursor(0, 1);
    lcd.print(airHumidity);
    lcd.print(" %   ");
}

// ── Fonctions de contrôle ──────────────────────────────
void humidifi() {
    delay(10);
    int  value_sol    = analogRead(capteur_humidite);
    long humidite_sol = (long)value_sol * 100 / 876;
    if (humidite_sol <= 45) {
        monservo_1.write(180);
    } else {
        monservo_1.write(90);
    }
}

void ouverture() {
    delay(10);
    int  sensor2  = analogRead(capteur_gaz);
    long sensor48 = (long)sensor2 * 100 / 380;
    if (sensor48 >= 55) {
        monservo_2.write(180);
    } else {
        monservo_2.write(0);
    }
}

void refroidir() {
    // Utilise la variable globale temperature (lue par DHT11)
    if (temperature >= 22) {
        digitalWrite(10, HIGH);
    } else {
        digitalWrite(10, LOW);
    }
}

void ajouter_lumunosite() {
    delay(10);
    int  valeur48          = analogRead(capteur_luminosite);
    long valeur_pourcentage = (long)valeur48 * 100 / 471;
    if (valeur_pourcentage <= 20) {
        digitalWrite(6, HIGH);
    } else {
        digitalWrite(6, LOW);
    }
}

void rechauffer() {
    // Utilise la variable globale temperature (lue par DHT11)
    if (temperature <= 18) {
        digitalWrite(13, HIGH);
    } else {
        digitalWrite(13, LOW);
    }
}

void limiter_humi_air() {
    // Utilise la variable globale airHumidity (lue par DHT11)
    if (airHumidity >= 45) {
        digitalWrite(10, HIGH);
    } else {
        digitalWrite(10, LOW);
    }
}

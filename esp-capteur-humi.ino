const int capteur_humi = 25; 
void setup() {
    Serial.begin(9600);
    pinMode(capteur_humi, OUTPUT);
    digitalWrite(capteur_humi, LOW);
}

void loop() {
    int val_humi_sol = analogRead(capteur_humi);
    int niveau = map(val_humi_sol, 0, 1023, 0, 100);
    Serial.print("Sensor value: ");
    Serial.print(niveau);
}
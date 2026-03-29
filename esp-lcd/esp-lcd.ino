#include <LiquidCrystal.h>

const int rs = 19, en = 18, d4 = 17, d5 = 16, d6 = 26, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
    Serial.begin(115200);
    lcd.begin(16, 2);
}

void loop() {
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Helloworld");
}
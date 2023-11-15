#include <EEPROM.h>
void setup() {
    Serial.println("start");
    int length = EEPROM.length();
    Serial.begin(9600);
    for (int i = 0; i < length; i++) {
        Serial.print(i);
        Serial.print("th byte is ");
        Serial.println(uint8_t(EEPROM.read(i)));
    }
    Serial.println("done");
    Serial.end();
}

void loop() {
}

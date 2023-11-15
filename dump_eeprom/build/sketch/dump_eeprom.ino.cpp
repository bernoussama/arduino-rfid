#include <Arduino.h>
#line 1 "/home/oussama/github/arduino-rfid/dump_eeprom/dump_eeprom.ino"
#include <EEPROM.h>
#line 2 "/home/oussama/github/arduino-rfid/dump_eeprom/dump_eeprom.ino"
void setup();
#line 15 "/home/oussama/github/arduino-rfid/dump_eeprom/dump_eeprom.ino"
void loop();
#line 2 "/home/oussama/github/arduino-rfid/dump_eeprom/dump_eeprom.ino"
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


/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

#include <EEPROM.h>

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int addr = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("Start");
    int length = EEPROM.length();
    for (int i = 0; i < length; i++) {
      EEPROM.write(i, 0);
    }
    Serial.println("nullified EEPROM");
    Serial.end();
}

void loop() {
 
}

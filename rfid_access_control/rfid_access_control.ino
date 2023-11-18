// adding access control system
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#include "pitches.h"


// Custom types
typedef byte idArray[4];  // defining idArray (an array of 4 bytes)

// Create MFRC instance
#define SS_PIN 10                  // slave select pin
#define RST_PIN 9                  // reset pin
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

#define BUILTIN_LED 13  // builtin led pin
#define RELAY_PIN 7     // relay pin
#define BUZZER_PIN 8    // buzzer pin

#define CARDS_COUNT EEPROM.read(0)     // number of cards in EEPROM
#define MAX_COUNT EEPROM.length() - 1  // 1023
#define DELAY_TIME 500                 // ms

bool programMode = false;  // initialize programming mode to false

idArray MASTER_CARD_UID = { 0xEC, 0x00, 0xD8, 0x32 };   // master Card ID
idArray MASTER_CARD_UID2 = { 0xCD, 0xED, 0xC9, 0x82 };  // master Card ID2
idArray readCard;                                       // Stores scanned ID read from RFID Module
idArray storedCard;                                     // Stores the ID read from EEPROM to check it
idArray nullID = { 0, 0, 0, 0 };                        // Empty ID array
int successRead;                                        // Variable to check if there was a successful read (MFRC522)
uint16_t idx;                                           // index in EEPROM of a found ID
uint16_t MAX_INDEX = MAX_COUNT - 3;                     // last index in EEPROM
unsigned long unlockStartTime = 0;                      // time when relay was unlocked
const uint32_t unlockDuration = 60000;                  // 1 minute is 60000 ms


namespace Tone {  // namespace for notes example: Tone::Do();

void Do(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_C1 * scale, 500);
};
void Re(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_D1 * scale, 500);
}
void Mi(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_E1 * scale, 500);
}
void Fa(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_F1 * scale, 500);
}
void Sol(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_G1 * scale, 500);
}
void La(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_A1 * scale, 500);
}
void Si(int scale = 1, int duration = 500) {
  tone(BUZZER_PIN, NOTE_B1 * scale, 500);
}

namespace eeprom {
// function that read 4 bytes and return an idArray
void read(uint16_t i, idArray *storedCard) {
  for (uint8_t x = 0; x < 4; x++) {
    *storedCard[x] = EEPROM.read(i + x);
  }
}
}

}
void setup() {
  blinkLED(3, 100);
  pinMode(BUILTIN_LED, OUTPUT);    // set led pin as output
  digitalWrite(BUILTIN_LED, LOW);  // Initialize led as off

  pinMode(BUZZER_PIN, OUTPUT);    // set buzzer pin as output
  digitalWrite(BUZZER_PIN, LOW);  // Initialize buzzer as off

  pinMode(RELAY_PIN, OUTPUT);    // set relay pin as output
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay as locked

  Serial.begin(9600);  // Initialize serial communications with the PC
  SPI.begin();         // SPI Protocol config

  // tone(BUZZER_PIN, 100, 100);  // beep to indicate start of setup
  // delay(100);
  // tone(BUZZER_PIN, 1000, 100);
  // noTone(BUZZER_PIN);
  // playMelody();
  // doremifasolasido();
  Tone::Si(5, 200);
  delay(200);
  Tone::Sol(4, 200);
  delay(200);
  Tone::Si(6, 200);
  delay(200);
  noTone(BUZZER_PIN);

  mfrc522.PCD_Init();  // Init MFRC522 card

  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details


  successRead = 0;  // Initialize success read to 0
  Serial.print("number of allowed cards :");
  Serial.print(CARDS_COUNT, DEC);
}


void loop() {
  // check for relay unlock timeout
  if (!isRelayLocked() && ((millis() - unlockStartTime) >= unlockDuration)) {
    lockRelay();
  }

  // if no ID is read it keeps checkin for cards
  successRead = getID();

  if (successRead) {

    if (programMode) {
      if (isMaster()) {  //When in program mode check First If master card scanned again to exit program mode
        Serial.println(F("Master Card Scanned"));
        Serial.println(F("Exiting Program Mode"));
        Serial.println(F("-----------------------------"));
        programMode = false;
        blinkLED(5, 50);
        return;
      } else {
        idx = findID(readCard);
        if (idx != 0) {  // If scanned card is known delete it
          Serial.println(idx);

          Serial.println(F("I know this PICC, removing..."));
          deleteID(idx);
          Serial.println(F("PICC REMOVED!"));

          Serial.println("-----------------------------");
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        } else {  // If scanned card is not known add it
          Serial.println(idx);

          Serial.println(F("I do not know this PICC, adding..."));
          writeID(readCard);
          Serial.println(F("PICC Added!"));
          Serial.println(F("-----------------------------"));
          Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        }
      }
    } else {
      if (isMaster()) {  // If scanned card's ID matches Master Card's ID - enter program mode
        programMode = true;
        blinkLED(5, 100);
        Serial.println(F("Hello Master - Entered Program Mode"));
        Serial.print(F("I have "));
        Serial.print(CARDS_COUNT);
        Serial.print(F(" record(s) on EEPROM"));
        Serial.println("");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
        Serial.println(F("Scan Master Card again to Exit Program Mode"));
        Serial.println(F("-----------------------------"));

      } else {  // If not, see if the card is in the EEPROM
        if (findID(readCard) != 0) {
          Serial.println(F("Welcome, You shall pass"));
          granted();  // grant access
          Serial.println(F("Access Granted!"));

        } else {  // If not, show that the ID was not valid
          Serial.println(F("You shall not pass"));
          denied();
          Serial.println(F("Access Denied!"));
        }
      }
    }
  }
  delay(DELAY_TIME);
}


///////////////////////// FUNCTIONS /////////////////////////////////


/**
* blink led
* @param times number of times to blink
* @param del delay between blinks
*/
void blinkLED(uint8_t times, uint8_t del) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(del);
    digitalWrite(BUILTIN_LED, LOW);
    delay(del);
  }
}

/**
* @return 1 if card is read, 0 if not
*/
int getID() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("");
    Serial.print("read card: ");
    for (uint8_t i = 0; i < 4; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
      Serial.print(readCard[i], HEX);
    }
    Serial.println("");
    return (1);
  } else {
    return (0);
  }
}
/**
* @return true if readCard is master card, false if not
*/
bool isMaster() {
  for (byte i = 0; i < 4; i++) {  // Check card ID read against master card
    if (readCard[i] != MASTER_CARD_UID[i] && readCard[i] != MASTER_CARD_UID2[i]) {
      return (false);
    }
  }
  return (true);
}


/**
* @return index of the id in EEPROM, 0 if not found
*/
uint16_t findID(idArray id) {
  if (CARDS_COUNT > 0) {
    for (uint16_t i = 1; i <= (CARDS_COUNT * 4) && i <= MAX_INDEX; i += 4) {
      idArray storedID;
      for (uint8_t x = 0; x < 4; x++) {
        storedID[x] = EEPROM.read(i + x);
      }
      if (compareID(id, storedID) == 1) {
        return i;
      }
    }
  }
  return 0;  // Return 0 if the ID is not found
}

/**
* write id to EEPROM
* @param id id to write to EEPROM
*/
void writeID(idArray id) {

  uint16_t newAddress;  // Calculate next available EEPROM address
  idArray tmpID;
  for (uint8_t i = 1; i < MAX_COUNT - 3; i + 4) {
    newAddress = i;
    for (uint8_t j = 0; j < 4; j++) {  // Write the new ID to EEPROM
      tmpID[j] = EEPROM.read(newAddress + j);
    }
    if (compareID(tmpID, nullID) == 1) {
      for (uint8_t j = 0; j < 4; j++) {
        EEPROM.write(newAddress + j, id[j]);
        EEPROM.write(0, uint8_t(CARDS_COUNT + 1));  // Increment the counter in the first address of EEPROM
      }
      break;
    }
  }
  Serial.println(F("PICC Added!"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
}
// else {
//   Serial.println(F("Reached maximum card capacity in EEPROM."));
// }
// }


/**
* delete id from EEPROM
* @param idx index of the id to delete
*/
void deleteID(uint16_t idx) {

  for (uint8_t i = 0; i < 4; i++) {
    EEPROM.write(idx + i, 0);
  }

  EEPROM.write(0, uint8_t(CARDS_COUNT - 1));  // decrement the counter in the first address of EEPROM
  Serial.println(F("PICC Removed!"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
}

/**
* read id from EEPROM
* @param i index of the id to read
*/
void readID(uint8_t i) {
  // EEPROM.get(i, storedCard);
  // use the other function below if you want to use storedCard as a char array
  for (uint8_t x = 0; x < 4; x++) {
    storedCard[x] = EEPROM.read(i + x);
  }
}

/**
* compare two ids
* @param id1 first id to compare
* @param id2 second id to compare
* @return 1 if ids are identical, 0 if not
*/
int compareID(idArray id1, idArray id2) {
  for (uint8_t i = 0; i < 4; i++) {
    if (id1[i] != id2[i])
      return (0);  // if a Byte is different
  }
  return (1);  // if identical
}


/**
* grant access
*/
void granted() {

  if (isRelayLocked()) {  // if relay is locked, unlock it
    unlockRelay();
    tone(BUZZER_PIN, 1000, 100);  // beep to indicate access granted
    tone(BUZZER_PIN, 500, 100);
  } else {  // if relay is unlocked, lock it
    lockRelay();
  }
}

/**
* deny access, do nothing
*/
void denied() {

  Tone::Si(5, 200);
  delay(200);
  Tone::Si(5, 200);
  delay(200);
  Tone::Si(5, 200);
  delay(200);
  noTone(BUZZER_PIN);
  ;
}

/**
* @return true if relay is locked, false if not
*/
bool isRelayLocked() {
  return (digitalRead(RELAY_PIN) == LOW);
}

/**
* unlock relay
*/
void unlockRelay() {
  digitalWrite(RELAY_PIN, HIGH);  // Unlock the relay
  unlockStartTime = millis();     // save unlock time
  Serial.println("Relay unlocked");
}

/**
* lock relay
*/
void lockRelay() {
  digitalWrite(RELAY_PIN, LOW);  // Lock the relay
  Serial.println("Relay locked");
}


/**
* Show details of PCD - MFRC522 Card Reader details
*/
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    while (true)
      ;  // do not go further
  }
}

void serialConsole() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'd') {
      dumpEEPROM();

    } else {
      Serial.println("unknown command");
    }
  }
}

char *readline() {
  static char line[80];
  static uint8_t lineIndex = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      line[lineIndex] = '\0';
      lineIndex = 0;
      return line;
    } else {
      line[lineIndex] = c;
      lineIndex++;
      if (lineIndex >= 80) {
        lineIndex = 0;
      }
    }
  }

  return NULL;
}

void dumpEEPROM() {
  Serial.println("dumping EEPROM");
  for (uint16_t i = 0; i <= MAX_COUNT; i++) {
    Serial.print(EEPROM.read(i), HEX);
    Serial.print(" ");
  }
  Serial.println("");
}
void clearEEPROM() {
  Serial.println("clearing EEPROM");
  for (uint16_t i = 0; i <= MAX_COUNT; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.write(0, 0);
}

void playMelody() {

  // notes in the melody:
  int melody[] = {

    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
  };

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {

    4, 8, 8, 4, 4, 4, 4, 4
  };
  // iterate over the notes of the melody:

  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.

    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.

    int noteDuration = 1000 / noteDurations[thisNote];

    tone(BUZZER_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.

    // the note's duration + 30% seems to work well:

    int pauseBetweenNotes = noteDuration * 1.30;

    delay(pauseBetweenNotes);

    // stop the tone playing:

    noTone(BUZZER_PIN);
  }
}
void doremifasolasido() {
  tone(BUZZER_PIN, NOTE_C4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_D4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_E4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_F4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_G4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_A4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_B4, 500);
  delay(500);
  tone(BUZZER_PIN, NOTE_C5, 500);
  delay(500);
  noTone(BUZZER_PIN);
}

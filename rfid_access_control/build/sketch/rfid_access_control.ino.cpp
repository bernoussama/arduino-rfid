#include <Arduino.h>
#line 1 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
// adding access control system
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>


// Custom types
typedef byte idArray[4];  // defining idArray (an array of 4 bytes)

// Create MFRC instance
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define RELAY_PIN 7
#define CARDS_COUNT EEPROM.read(0)
#define MAX_COUNT EEPROM.length() - 1

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
const uint32_t unlockDuration = 6000;                   // 1 minute is 60000 ms

#line 32 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void setup();
#line 51 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void loop();
#line 128 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
int getID();
#line 143 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
bool isMaster();
#line 155 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
uint16_t findID(idArray id);
#line 185 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void writeID(idArray id);
#line 208 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void deleteID(uint16_t idx);
#line 224 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void readID(uint8_t i);
#line 238 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
int compareID(idArray id1, idArray id2);
#line 254 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void granted();
#line 266 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void denied();
#line 273 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
bool isRelayLocked();
#line 280 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void unlockRelay();
#line 289 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void lockRelay();
#line 298 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void ShowReaderDetails();
#line 32 "/home/oussama/github/arduino-rfid/rfid_access_control/rfid_access_control.ino"
void setup() {

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay as locked

  Serial.begin(9600);
  // SPI Protocol config
  SPI.begin();

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
  delay(1000);
}


///////////////////////// FUNCTIONS /////////////////////////////////


/**
* @return 1 if card is read, 0 if not
*/
int getID() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
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
  for (byte i = 0; i < 4; i++) {
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
      ////////////////////
      Serial.print("id: ");
      for (uint8_t x = 0; x < 4; x++) {
        Serial.print(id[x], HEX);
      }
      Serial.print("storedID: ");
      for (uint8_t x = 0; x < 4; x++) {
        Serial.print(storedID[x], HEX);
      }
      Serial.println("");
      /////////////////////
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
  if (CARDS_COUNT < (MAX_COUNT / 4)) {  // Ensure there's space for the new ID

    uint16_t newAddress = (CARDS_COUNT * 4) + 1;  // Calculate next available EEPROM address
    // EEPROM.put(newAddress, id);  // Write the new ID to EEPROM
    for (uint8_t i = 0; i < 4; i++) {  // Write the new ID to EEPROM
      EEPROM.write(newAddress + i, id[i]);
    }
    EEPROM.write(0, uint8_t(CARDS_COUNT + 1));  // Increment the counter in the first address of EEPROM

    Serial.println(F("PICC Added!"));
    Serial.println(F("-----------------------------"));
    Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
  } else {
    Serial.println(F("Reached maximum card capacity in EEPROM."));
  }
}


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
    Serial.print("id: ");
    Serial.println(id1[i], HEX);
    Serial.print("storedCard: ");
    Serial.println(id2[i], HEX);
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
  } else {  // if relay is unlocked, lock it
    lockRelay();
  }
}

/**
* deny access, do nothing
*/
void denied() {
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


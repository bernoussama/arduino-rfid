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
#define MAX_COUNT EEPROM.length()

bool programMode = false;  // initialize programming mode to false

idArray MASTER_CARD_UID = { 0xEC, 0x00, 0xD8, 0x32 };  // master Card ID
idArray readCard;                                      // Stores scanned ID read from RFID Module
idArray storedCard;                                    // Stores the ID read from EEPROM to check it
idArray nullID = { 0, 0, 0, 0 };
int successRead;
uint16_t idx;  // index in EEPROM of a found ID
uint16_t MAX_INDEX = MAX_COUNT - 4;
unsigned long unlockStartTime = 0;
const uint32_t unlockDuration = 6000;  // 1 minute is 60000 ms

void setup() {

  Serial.begin(9600);
  // SPI Protocol config
  SPI.begin();
  mfrc522.PCD_Init();

  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay as locked

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
        if ((idx = findID(readCard))) {  // If scanned card is known delete it
          Serial.println(idx);

          Serial.println(F("I know this PICC, removing..."));
          deleteID();
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

      } else {
        if (findID(readCard)) {  // If not, see if the card is in the EEPROM
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
    delay(1000);
  }
}

//// Get card ID ////
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
bool isMaster() {
  for (byte i = 0; i < 4; i++) {
    if (readCard[i] != MASTER_CARD_UID[i]) {
      return (false);
    }
  }
  return (true);
}

bool isRelayLocked() {
  return (digitalRead(RELAY_PIN) == LOW);
}

void unlockRelay() {
  digitalWrite(RELAY_PIN, HIGH);  // Unlock the relay
  unlockStartTime = millis();
  Serial.println("Relay unlocked");
}

void lockRelay() {
  digitalWrite(RELAY_PIN, LOW);  // Lock the relay
  Serial.println("Relay locked");
}

uint16_t findID(idArray id) {
  if (CARDS_COUNT > 0) {
    for (uint16_t i = 1; i <= (CARDS_COUNT * 4) && i <= MAX_INDEX; i += 4) {
      idArray storedID;
      EEPROM.get(i, storedID);
      if (compareID(id, storedID) == 1) {
        return i;
      }
    }
  }
  return 0;  // Return 0 if the ID is not found
}


// uint16_t findID(idArray id) {
//   if (CARDS_COUNT > 0) {
//     for (uint8_t i = 1; i <= (CARDS_COUNT * 4) && i <= MAX_INDEX; i++) {
//       readID(i);  // Read an ID from EEPROM, it is stored in storedCard[4]
//       if (compareID(id, storedCard) != 0) {
//         return (i);
//       }
//     }
//     return (0);
//   } else
//     return (0);
// }

void writeID(idArray id) {
  if (CARDS_COUNT < (MAX_COUNT / 4)) {            // Ensure there's space for the new ID
    uint16_t newAddress = (CARDS_COUNT * 4) + 1;  // Calculate next available EEPROM address
    EEPROM.put(newAddress, id);                   // Write the new ID to EEPROM
    EEPROM.write(0, uint8_t(CARDS_COUNT + 1));
    Serial.println(F("PICC Added!"));
    Serial.println(F("-----------------------------"));
    Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
  } else {
    Serial.println(F("Reached maximum card capacity in EEPROM."));
  }
}

// void writeID(idArray id) {
//   if (CARDS_COUNT < (MAX_COUNT / 4)) {  // Ensure there's space for the new ID
//     for (uint16_t i = 1; i <= MAX_INDEX; i++) {
//       Serial.print("EEPROM ");
//       Serial.print(i);
//       Serial.print(" : ");
//       Serial.println(EEPROM[i], DEC);
//       idArray storedID;
//       EEPROM.get(i, storedID);
//       if (compareID(storedID, nullID) == 1) {
//         EEPROM.put(i, id);
//         Serial.print(i);
//         Serial.println("");
//         Serial.print(*EEPROM.get(i, storedCard), HEX);
//         Serial.println("");
//         EEPROM.write(0, uint8_t(CARDS_COUNT + 1));
//         return;
//       }
//     }
//     Serial.println(F("EEPROM is full! Cannot add more cards."));
//   } else {
//     Serial.println(F("Reached maximum card capacity in EEPROM."));
//   }
// }


void deleteID() {
  uint16_t cardIndex = idx / 4;                    // Calculate the card index based on EEPROM address
  uint16_t nextCardAddress = (cardIndex + 1) * 4;  // Calculate address of the next card

  // Move the subsequent cards to fill the gap left by the deleted card
  for (uint16_t i = nextCardAddress; i <= (CARDS_COUNT * 4) && i <= MAX_INDEX; i += 4) {
    idArray nextCard;
    EEPROM.get(i, nextCard);
    EEPROM.put(i - 4, nextCard);
  }

  EEPROM.put(CARDS_COUNT * 4, nullID);  // Clear the last card slot
  EEPROM.write(0, uint8_t(CARDS_COUNT - 1));
  EEPROM.write(0, CARDS_COUNT);  // Update the count of stored cards

  Serial.println(F("PICC Removed!"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
}

// void deleteID() {
//   EEPROM.put(idx, nullID);
//   EEPROM.write(0, uint8_t(CARDS_COUNT - 1));
// }

void readID(uint8_t i) {
  EEPROM.get(i, storedCard);
}

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

void granted() {
  if (isRelayLocked()) {
    unlockRelay();
    delay(500);
  } else {
    lockRelay();
    delay(500);
  }
}

void denied() {
  ;
}

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

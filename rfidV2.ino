#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);

#define RELAY_PIN 7

byte MASTER_CARD_UID[4] = { 0xEC, 0x00, 0xD8, 0x32 };
byte MASTER_CARD2_UID[4] = { 0x83, 0x46, 0xF9, 0x14 };
unsigned long unlockStartTime = 0;
const unsigned long unlockDuration = 6000;  // 1 minute is 60000 ms

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay as locked
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    byte cardUID[4];
    for (byte i = 0; i < 4; i++) {
      cardUID[i] = mfrc522.uid.uidByte[i];
    }

    if (isMasterCard(cardUID)) {
      if (isRelayLocked()) {
        unlockRelay();
        delay(500);
      } else {
        lockRelay();
        delay(500);

      }
    }
  }

  if (!isRelayLocked() && millis() - unlockStartTime >= unlockDuration) {
    lockRelay();
  }
}

bool isMasterCard(byte cardUID[4]) {
  for (byte i = 0; i < 4; i++) {
    if (cardUID[i] != MASTER_CARD_UID[i] && cardUID[i] != MASTER_CARD2_UID[i]) {
      return false;
    }
  }
  return true;
}

bool isRelayLocked() {
  return digitalRead(RELAY_PIN) == LOW;
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

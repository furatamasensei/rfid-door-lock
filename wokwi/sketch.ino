/*
  RFID Door Lock — Wokwi simulator
  Arduino Uno + MFRC522 + servo latch + buzzer + status LEDs

  This is the no-hardware version of the project:
  - the servo stands in for the 12V solenoid latch
  - the MFRC522 control panel provides virtual RFID cards
  - the Blue Card (UID 01 02 03 04) is authorized
  - all other cards are denied

  Circuito.io wiring for the real build is in the parent README.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN     10
#define RST_PIN     9
#define SERVO_PIN   3
#define BUZZER_PIN  2
#define GREEN_LED   5
#define RED_LED     6

const byte AUTHORIZED_UID[] = {0x01, 0x02, 0x03, 0x04};  // Blue Card
const unsigned long UNLOCK_MS = 3000;
const int LOCKED_ANGLE = 0;
const int UNLOCKED_ANGLE = 90;

MFRC522 rfid(SS_PIN, RST_PIN);
Servo lockServo;

void printUid(byte *uid, byte size) {
  for (byte i = 0; i < size; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
}

bool isAuthorized(byte *uid, byte size) {
  if (size != sizeof(AUTHORIZED_UID)) {
    return false;
  }
  for (byte i = 0; i < size; i++) {
    if (uid[i] != AUTHORIZED_UID[i]) {
      return false;
    }
  }
  return true;
}

void grantAccess() {
  Serial.println(F("ACCESS GRANTED"));
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  tone(BUZZER_PIN, 1800, 80);
  delay(110);
  tone(BUZZER_PIN, 2400, 80);
  delay(110);

  lockServo.write(UNLOCKED_ANGLE);
  delay(UNLOCK_MS);
  lockServo.write(LOCKED_ANGLE);

  digitalWrite(GREEN_LED, LOW);
  Serial.println(F("LOCKED"));
}

void denyAccess() {
  Serial.println(F("ACCESS DENIED"));
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);

  tone(BUZZER_PIN, 350, 600);
  delay(650);
  digitalWrite(RED_LED, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCKED_ANGLE);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println(F("RFID Door Lock — Wokwi"));
  Serial.println(F("Tap the BLUE CARD (UID 01 02 03 04)"));
  Serial.println(F("Other cards are denied."));

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("MFRC522 version: 0x"));
  Serial.println(version, HEX);
  if (version == 0x00 || version == 0xFF) {
    Serial.println(F("RFID reader not detected. Check wiring."));
  }

  Serial.println(F("Ready."));
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("Card UID:"));
  printUid(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
    grantAccess();
  } else {
    denyAccess();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(800);
}

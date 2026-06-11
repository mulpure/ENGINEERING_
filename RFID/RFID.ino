#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>
#include <SoftwareSerial.h>

// ===== 핀 정의 =====
#define RST_PIN   9
#define SS_PIN    10
#define IN1       2
#define IN2       3
#define IN3       4
#define IN4       5
#define LED_PIN   8
#define BUZZ_PIN  A5
#define BT_RX     7
#define BT_TX     6

#define STEPS_PER_REV  2048
#define STEPS_90DEG    (STEPS_PER_REV / 4)

Stepper stepper(STEPS_PER_REV, IN1, IN3, IN2, IN4);
MFRC522 mfrc(SS_PIN, RST_PIN);
SoftwareSerial btSerial(BT_RX, BT_TX);

byte registeredUID[] = {0xEE, 0xBD, 0xFF, 0x06};

void releaseMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

bool checkMatch(byte *uid1, byte *uid2) {
  for (byte i = 0; i < 4; i++)
    if (uid1[i] != uid2[i]) return false;
  return true;
}

void printUID(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}

void openDoor() {
  Serial.println("[동작] 문 열림");
  btSerial.println("DOOR_OPENING");

  stepper.step(STEPS_90DEG);
  releaseMotor();
  digitalWrite(LED_PIN, HIGH);
  btSerial.println("DOOR_OPEN");

  for (int i = 7; i >= 1; i--) {
    btSerial.print("CLOSE_IN_");
    btSerial.println(i);
    delay(1000);
  }

  Serial.println("[동작] 문 닫힘");
  digitalWrite(LED_PIN, LOW);
  btSerial.println("DOOR_CLOSING");

  stepper.step(-STEPS_90DEG);
  releaseMotor();
  btSerial.println("DOOR_CLOSED");
}

void setup() {
  Serial.begin(9600);
  btSerial.begin(9600);
  SPI.begin();
  mfrc.PCD_Init();

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(LED_PIN,  OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);

  digitalWrite(LED_PIN,  LOW);
  digitalWrite(BUZZ_PIN, LOW);
  releaseMotor();
  stepper.setSpeed(10);

  Serial.println("--- 시스템 준비 완료 ---");
  btSerial.println("SYSTEM_READY");
}

void loop() {
  // ── BT 명령 수신 ──
  if (btSerial.available()) {
    String cmd = btSerial.readStringUntil('\n');
    cmd.trim();
    cmd.replace("\r", "");
    cmd.replace("\n", "");

    Serial.print("[BT 수신] '");
    Serial.print(cmd);
    Serial.println("'");

    // ↓ '0' 추가 (앱이 "0"을 보내는 경우 처리)
    if (cmd == "O" || cmd.equalsIgnoreCase("OPEN") || cmd.indexOf("OPEN") >= 0) {
      btSerial.println("BT_CMD_RECEIVED");
      openDoor();
    }
  }

  // ── RFID 카드 인식 ──
  if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  Serial.print("\n[카드 인식] UID:");
  printUID(mfrc.uid.uidByte);
  btSerial.println("CARD_DETECTED");

  if (checkMatch(mfrc.uid.uidByte, registeredUID)) {
    Serial.println("[성공] 인증 완료");
    btSerial.println("ACCESS_GRANTED");
    openDoor();
  } else {
    Serial.println("[실패] 미인증 카드");
    btSerial.println("ACCESS_DENIED");
    digitalWrite(BUZZ_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZ_PIN, LOW);
  }

  mfrc.PICC_HaltA();
  delay(1000);
}
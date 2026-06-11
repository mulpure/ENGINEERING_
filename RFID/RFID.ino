#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>

#define RST_PIN  9
#define SS_PIN   10
#define IN1  2
#define IN2  3
#define IN3  4
#define IN4  5
#define LED_PIN   8
#define BUZZ_PIN  A5                  // ← 추가

#define STEPS_PER_REV  2048
#define STEPS_90DEG    (STEPS_PER_REV / 4)

Stepper stepper(STEPS_PER_REV, IN1, IN3, IN2, IN4);
MFRC522 mfrc(SS_PIN, RST_PIN);

byte registeredUID[] = {0xEE, 0xBD, 0xFF, 0x06};

void releaseMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

bool checkMatch(byte *uid1, byte *uid2) {
  for (byte i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void printUID(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc.PCD_Init();
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(LED_PIN,  OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);          // ← 추가
  digitalWrite(LED_PIN,  LOW);
  digitalWrite(BUZZ_PIN, LOW);        // ← 추가
  releaseMotor();
  stepper.setSpeed(10);
  Serial.println("--- 시스템 준비 완료 ---");
  Serial.print("등록된 카드 UID:");
  printUID(registeredUID);
}

void loop() {
  if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  Serial.print("\n[카드 인식] UID:");
  printUID(mfrc.uid.uidByte);

  if (checkMatch(mfrc.uid.uidByte, registeredUID)) {
    Serial.println("[성공] 문 열림");
    stepper.step(STEPS_90DEG);
    releaseMotor();
    digitalWrite(LED_PIN, HIGH);

    delay(5000);

    Serial.println("[복귀] 문 닫힘");
    digitalWrite(LED_PIN, LOW);
    stepper.step(-STEPS_90DEG);
    releaseMotor();
  } else {
    Serial.println("[실패] 미인증 카드");
    digitalWrite(BUZZ_PIN, HIGH);     // ← 부저 켜기
    delay(1000);                      // ← 1초 울림
    digitalWrite(BUZZ_PIN, LOW);      // ← 부저 끄기
  }

  mfrc.PICC_HaltA();
  delay(1000);
}
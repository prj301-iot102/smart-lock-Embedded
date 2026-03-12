#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h> // 1. Include the Servo library

#define RST_PIN         9          
#define SS_PIN          10         
#define SERVO_PIN       3  // Your SG90 is on Pin D3

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo myServo;        // 2. Create the servo object

unsigned long lastCheckTime = 0;
const int checkInterval = 3000;

void setup() {
  Serial.begin(9600);
  while (!Serial); 
  SPI.begin();           
  
  mfrc522.PCD_Init();    
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  
  myServo.attach(SERVO_PIN); // 3. Connect the servo to the pin
  myServo.write(0);          // Set initial position to 0 degrees
  
  Serial.println(F("--- RC522 Monitor Started ---"));
  Serial.println(F("Ready to swipe..."));
}

void loop() {
  // --- PART 1: ALIVE CHECK ---
  if (millis() - lastCheckTime > checkInterval) {
    lastCheckTime = millis();
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
      Serial.println(F("ALIVE CHECK: FAILED! Check your wiring."));
    } else {
      Serial.print(F("ALIVE CHECK: OK (Version 0x"));
      Serial.print(v, HEX);
      Serial.println(F(")"));
    }
  }

  // --- PART 2: SWIPE CHECK ---
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  if ( ! mfrc522.PICC_ReadCardSerial()) return;

  // SUCCESS!
  Serial.println(F("******************************"));
  Serial.println(F("         SUCCESS!             "));
  
  // --- PART 3: SERVO ACTION ---
  Serial.println(F("   Unlocking (90°)..."));
  myServo.write(90);    // Move to 90 degrees
  delay(2000);          // Wait 2 seconds
  
  Serial.println(F("   Locking (0°)..."));
  myServo.write(0);     // Return to 0 degrees
  
  Serial.println(F("******************************"));

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
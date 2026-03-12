#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#define RST_PIN         9          
#define SS_PIN          10         
#define BUZZER_PIN      2   
#define SERVO_PIN       3  

// SoftwareSerial(RX, TX) -> RX connects to ESP TX (D6), TX connects to ESP RX (D5)
SoftwareSerial espSerial(6, 5); 

MFRC522 mfrc522(SS_PIN, RST_PIN);  
Servo myServo;

void setup() {  
  Serial.begin(9600);    // For debugging
  espSerial.begin(9600); // For talking to ESP8266
  SPI.begin();  
  mfrc522.PCD_Init();    
  myServo.attach(SERVO_PIN);  
  pinMode(BUZZER_PIN, OUTPUT);
  myServo.write(0);   
  Serial.println("Ready. Scan Card...");
}

void loop() {
  // --- PART A: Check for Background Commands from ESP (like <CR> or <VA>) ---
  if (espSerial.available()) {
    char c = espSerial.read();
    if (c == '<') {
      delay(10); // Let the buffer fill
      char command = espSerial.read();
      
      if (command == 'C') { // Received <CR> from ESP
        creatingBeep();
      }
      // You can add more background commands here if needed
    }
  }

  // --- PART B: Check for New NFC Card ---
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  // 1. Get UID (No spaces)
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if(mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uid.toUpperCase();
  
  // 2. Send to ESP8266 via Serial
  Serial.println("Sent to ESP: " + uid);
  espSerial.println(uid); 

  // 3. Wait for validation response (Locking loop for feedback)
  unsigned long startWait = millis();
  bool responseReceived = false;

  while (millis() - startWait < 5000) { 
    if (espSerial.available()) {
      char c = espSerial.read();
      if (c == '<') { 
        delay(10); 
        char command = espSerial.read(); 

        if (command == '1') {
          openDoor();
          responseReceived = true;
          break;
        } else if (command == '0') {
          denyAccess();
          responseReceived = true;
          break;
        } else if (command == 'S') { // <SC>
          successCreateBeep();
          responseReceived = true;
          break;
        } else if (command == 'F') { // <FC>
          failCreateBeep();
          responseReceived = true;
          break;
        } 
      }
    }
  }

  if(!responseReceived) Serial.println("Timeout: No response from ESP");

  mfrc522.PICC_HaltA();  
  mfrc522.PCD_StopCrypto1();
}

void openDoor() {
  Serial.println("Access Granted!");
  digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
  myServo.write(90); delay(3000); 
  myServo.write(0);
}

void denyAccess() {
  Serial.println("Access Denied!");
  for(int i=0; i<3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100);
  }
}

void successCreateBeep() {
  Serial.println("Create Successful!");
  for(int i=0; i<2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(600); // Long beep
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void failCreateBeep() {
  Serial.println("Create Failed!");
  for(int i=0; i<3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(600); // Long beep
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void creatingBeep() {
  Serial.println("Creating");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(600); // Long beep
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
}
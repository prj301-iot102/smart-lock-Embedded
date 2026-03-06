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
  espSerial.begin(115200); // For talking to ESP8266
  SPI.begin();  
  mfrc522.PCD_Init();    
  myServo.attach(SERVO_PIN);  
  pinMode(BUZZER_PIN, OUTPUT);
  myServo.write(0);   
  Serial.println("Ready. Scan Card...");
}

void loop() {  
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

  // 3. Wait for ESP8266 validation response
  unsigned long startWait = millis();
  bool responseReceived = false;
  while (millis() - startWait < 10000) {
    if (espSerial.available()) {
      char c = espSerial.read();
      Serial.print("ESP_DEBUG: 0x");
      if (c < 0x10) Serial.print("0");
      Serial.print(c, HEX);
      Serial.print(" (");
      if (c >= 32 && c <= 126) Serial.write(c); // Print as char if printable
      else Serial.print("NonPrint");
      Serial.println(")");
      if (c == '<') { // Look for the start of the packet
        char command = espSerial.read(); // Read the actual command
        if (command == '1') {
          openDoor();
          responseReceived = true;
        } else if (command == '0') {
          denyAccess();
          responseReceived = true;
        }
        break; // Exit the loop after processing
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
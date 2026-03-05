#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Pins for RFID
#define RST_PIN         9          
#define SS_PIN          10         
// Pins for Peripherals
#define SERVO_PIN       3  
#define SPEAKER_PIN     2  
// Pins for ESP8266 Communication
#define ARDUINO_RX      5  // Connect to ESP TX
#define ARDUINO_TX      6  // Connect to ESP RX

// Setup SoftwareSerial to talk to ESP8266
SoftwareSerial espSerial(ARDUINO_RX, ARDUINO_TX); 

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo myServo;

void setup() {
  Serial.begin(9600);    // For your Computer Monitor
  espSerial.begin(9600); // For the ESP8266
  
  SPI.begin();           
  mfrc522.PCD_Init();    
  
  myServo.attach(SERVO_PIN);
  myServo.write(120);    // Initial position (Closed)
  
  pinMode(SPEAKER_PIN, OUTPUT);
  
  Serial.println(F("SYSTEM ONLINE: Ready to scan..."));
}

void loop() {
  // 1. Listen for feedback from ESP8266
  if (espSerial.available()) {
    String feedback = espSerial.readStringUntil('\n');
    Serial.print("ESP8266 says: ");
    Serial.println(feedback);
  }

  // 2. Look for new RFID cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  if ( ! mfrc522.PICC_ReadCardSerial()) return;

  // --- ACCESS GRANTED ACTION ---
  Serial.println(F("CARD DETECTED!"));

  // Send signal to ESP8266
  // This tells the ESP "Hey, do your Wi-Fi thing!"
  espSerial.println("CARD_SCANNED"); 
  
  // Audio/Visual Feedback
  tone(SPEAKER_PIN, 1000, 200); 
  myServo.write(20);            // Open door
  
  delay(3000);                  // Keep open for 3 seconds
  
  myServo.write(120);           // Close door
  tone(SPEAKER_PIN, 800, 100);  // Double beep
  delay(150);
  tone(SPEAKER_PIN, 800, 100);
  
  Serial.println(F("DOOR CLOSED"));

  // Reset RFID for next scan
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
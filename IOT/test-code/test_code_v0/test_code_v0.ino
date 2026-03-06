#include <SPI.h>
#include <MFRC522.h>

#include <Servo.h>
#define RST_PIN         9         
#define SS_PIN          10         
#define BUZZER_PIN      2   
#define SERVO_PIN       3  
MFRC522 mfrc522(SS_PIN, RST_PIN);  
Servo myServo;
void setup() {  
	Serial.begin(9600);  
	SPI.begin();  
	mfrc522.PCD_Init();    
	myServo.attach(SERVO_PIN);  
	pinMode(BUZZER_PIN, OUTPUT);
  myServo.write(0);   
  Serial.println("He thong san sang! Quet the bat ky de mo...");
}
void loop() {  
	if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {    
		return;  }
  Serial.println("The hop le: Mo cua!");   
  digitalWrite(BUZZER_PIN, HIGH);  
  delay(200);  
  digitalWrite(BUZZER_PIN, LOW);    
  myServo.write(90);     delay(3000); 
  Serial.println("Tu dong dong cua...");    
  for(int i = 0; i < 2; i++) {    
  digitalWrite(BUZZER_PIN, HIGH);    
  delay(150);    
  digitalWrite(BUZZER_PIN, LOW);    
  delay(100);  }    
  myServo.write(0); 
  mfrc522.PICC_HaltA();  
  mfrc522.PCD_StopCrypto1();
}
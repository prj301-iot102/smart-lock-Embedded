
#define RED_PIN    5  
#define GREEN_PIN  4  
#define BLUE_PIN   13 

void setup() {
  Serial.begin(9600); 
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message == "CARD_SCANNED") {
      digitalWrite(GREEN_PIN, HIGH);
      delay(2000);
      digitalWrite(GREEN_PIN, LOW);
    }
  }
}
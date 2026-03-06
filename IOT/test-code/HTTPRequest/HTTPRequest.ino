#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid     = "Patohru";
const char* password = "khoai)()*";

const char* BASE_URL = "http://192.168.0.10:8080";
const char* FLAG_ENDPOINT = "/api/devices/flag";
const char* CREATE_ENDPOINT = "/api/nfc/create";
const char* VALIDATE_ENDPOINT = "/api/nfc/validate";

enum State {
  CHECK_FLAG,
  WAIT_FOR_NFC_CREATE,
  WAIT_FOR_NFC_VALIDATE
};

State currentState = CHECK_FLAG;

unsigned long lastFlagCheck = 0;
const unsigned long FLAG_INTERVAL = 10000;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

String getMacAddress() {
  return WiFi.macAddress();
}

bool checkFlag() {
  WiFiClient client;
  HTTPClient http;

  String url = String(BASE_URL) + FLAG_ENDPOINT;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"mac_address\":\"" + getMacAddress() + "\"}";
  Serial.println(getMacAddress());
  int httpCode = http.POST(body);

  bool flagResult = false;

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Flag response: " + response);

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      flagResult = doc.as<bool>();
    }
  } else {
    Serial.println("Flag request failed: " + http.errorToString(httpCode));
  }

  http.end();
  return flagResult;
}

void createNfc(String uid) {
  WiFiClient client;
  HTTPClient http;

  String url = String(BASE_URL) + CREATE_ENDPOINT;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"uid\":\"" + uid + "\",\"mac_address\":\"" + getMacAddress() + "\"}";
  Serial.println("📤 Creating NFC: " + body);

  int httpCode = http.POST(body);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("NFC Created! Response: " + response);
  } else {
    Serial.println("Create failed: " + http.errorToString(httpCode));
  }

  http.end();
}

bool validateNfc(String uid) {
  WiFiClient client;
  HTTPClient http;

  String url = String(BASE_URL) + VALIDATE_ENDPOINT;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"uid\":\"" + uid + "\",\"mac_device\":\"" + getMacAddress() + "\"}";
  Serial.println("Validating NFC: " + body);

  bool result = false;

  int httpCode = http.POST(body);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Validate response: " + response);

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      result = doc.as<bool>();
    }
  } else {
    Serial.println("Validate failed: " + http.errorToString(httpCode));
  }

  http.end();
  return result;
}

void loop() {
  unsigned long now = millis();
  if (now - lastFlagCheck >= FLAG_INTERVAL) {
    lastFlagCheck = now;
    Serial.println("\nChecking flag...");

    bool flagOn = checkFlag();
    if (flagOn) {
      Serial.println("Flag TRUE → Waiting for NFC to CREATE...");
      currentState = WAIT_FOR_NFC_CREATE;
    } else {
      Serial.println("Flag FALSE → Waiting for NFC to VALIDATE...");
      currentState = WAIT_FOR_NFC_VALIDATE;
    }
  }

  switch (currentState) {
    case WAIT_FOR_NFC_CREATE: {
      if (Serial.available()) {
        String uid = Serial.readStringUntil('\n');
        uid.trim();
        if (uid.length() > 0) {
          Serial.println("UID received: " + uid);
          createNfc(uid);
          currentState = CHECK_FLAG;
        }
      }
      break;
    }
    case WAIT_FOR_NFC_VALIDATE: {
      if (Serial.available()) {
        String uid = Serial.readStringUntil('\n');
        uid.trim();
        if (uid.length() > 0) {
          bool isValid = validateNfc(uid);
          
          // Send the response multiple times or with a newline to ensure it's caught
          if (isValid) {
            Serial.println("1"); 
          } else {
            Serial.println("0");
          }
          currentState = CHECK_FLAG;
        }
      }
      break;
    }
  }
}
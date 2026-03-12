#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid     = "Pixel_9218";
const char* password = "888592996@";

const char* BASE_URL          = "http://smart-lock.patohru.qzz.io";
const char* FLAG_ENDPOINT     = "/api/devices/flag";
const char* CREATE_ENDPOINT   = "/api/nfc/create";
const char* VALIDATE_ENDPOINT = "/api/nfc/validate";

enum State { WAIT_FOR_NFC_CREATE, WAIT_FOR_NFC_VALIDATE };
State currentState = WAIT_FOR_NFC_VALIDATE;

unsigned long lastFlagCheck = 0;
const unsigned long FLAG_INTERVAL = 5000;

// Non-blocking serial buffer
String serialBuffer = "";
unsigned long lastCharTime = 0;
const unsigned long SERIAL_WATCHDOG = 500;

// ── WiFi ──────────────────────────────────────────────────
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("WiFi lost, reconnecting...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "Reconnected!" : "WiFi failed!");
}

// ── Non-blocking serial read ──────────────────────────────
String readSerialLine() {
  if (serialBuffer.length() > 0 && millis() - lastCharTime > SERIAL_WATCHDOG) {
    Serial.println("Watchdog cleared: " + serialBuffer);
    serialBuffer = "";
  }

  while (Serial.available()) {
    char c = Serial.read();
    lastCharTime = millis();
    if (c == '\n') {
      String line = serialBuffer;
      serialBuffer = "";
      line.trim();
      return line;
    }
    serialBuffer += c;
  }
  return "";
}

String getMacAddress() {
  return WiFi.macAddress();
}

// ── HTTP ──────────────────────────────────────────────────
bool checkFlag() {
  ensureWiFi();
  WiFiClient client;
  HTTPClient http;

  http.begin(client, String(BASE_URL) + FLAG_ENDPOINT);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"mac_address\":\"" + getMacAddress() + "\"}";
  int httpCode = http.POST(body);

  bool flagResult = false;
  if (httpCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(256);
    if (!deserializeJson(doc, response)) {
      flagResult = doc.as<bool>();
    }
  } else {
    Serial.println("Flag failed: " + http.errorToString(httpCode));
  }

  http.end();
  client.stop();
  return flagResult;
}

void createNfc(String uid) {
  ensureWiFi();
  WiFiClient client;
  HTTPClient http;

  http.begin(client, String(BASE_URL) + CREATE_ENDPOINT);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"uid\":\"" + uid + "\",\"mac_address\":\"" + getMacAddress() + "\"}";
  int httpCode = http.POST(body);

  if (httpCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(256);
    bool ok = false;
    if (!deserializeJson(doc, response)) {
      ok = doc.as<bool>();
    }
    Serial.println(ok ? "<SC>" : "<FC>");
  } else {
    Serial.println("<FC>");
  }
  http.end();
  client.stop();
}

bool validateNfc(String uid) {
  ensureWiFi();
  WiFiClient client;
  HTTPClient http;

  http.begin(client, String(BASE_URL) + VALIDATE_ENDPOINT);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"uid\":\"" + uid + "\",\"mac_device\":\"" + getMacAddress() + "\"}";

  bool result = false;
  int httpCode = http.POST(body); 
  
  // REMOVE the debug print here!
  
  if (httpCode == 200) { // Check for 200 specifically
    String response = http.getString();
    DynamicJsonDocument doc(256);
    if (!deserializeJson(doc, response)) {
      result = doc.as<bool>();
    }
  }

  http.end();
  return result;
}

// ── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  Serial.println("MAC: " + getMacAddress());
}

// ── Loop ──────────────────────────────────────────────────
void loop() {

  // ── 1. Serial read ALWAYS first ──
  String uid = readSerialLine();

  // ── 2. If card swiped, handle immediately ──
  if (uid.length() > 0) {
    Serial.println("UID received: " + uid);

    if (currentState == WAIT_FOR_NFC_CREATE) {
      createNfc(uid);
    } else {
      bool isValid = validateNfc(uid);
      Serial.println(isValid ? "<1>" : "<0>");
    }
  }

  // ── 3. Flag check only when no swipe is being processed ──
  else if (millis() - lastFlagCheck >= FLAG_INTERVAL) {
    lastFlagCheck = millis();

    bool flagOn = checkFlag();
    State newState = flagOn ? WAIT_FOR_NFC_CREATE : WAIT_FOR_NFC_VALIDATE;
    Serial.println(flagOn ? "<CR>" : "<VA>");
    if (newState != currentState) {
      currentState = newState;
      
    }
  }
}

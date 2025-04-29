#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char* ssid = "Wokwi-GUEST";       
const char* password = "";

// ===== OLED Setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin (not used)

#define I2C_SDA 17
#define I2C_SCL 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* BASE_URL = "https://api-lock-service-1046300342556.us-central1.run.app";
const char* API_KEY = "api123";

// ===== Keypad Setup =====
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {11, 12, 13, 14};  // safer GPIOs for ESP32-S3
byte colPins[COLS] = {6, 7, 8, 9}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===== Servo Setup =====
Servo myServo;
const int servoPin = 10;

String currentPin = "";
bool doorOpen = false;

void connectToWiFi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  // Aguarda até conectar
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Tentando conectar...");
  }

  Serial.println("Conectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP()); // Mostra o IP atribuído ao ESP32
}

void displayMessage(const char* line1, const char* line2 = "") {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println(line2);
  display.display();
}

void openDoor() {
  myServo.write(180);  // Open position
  doorOpen = true;
  displayMessage("DOOR OPEN", "Closing in 5s");
  Serial.println("Door opened");
  delay(5000);
  myServo.write(0);    // Closed position
  doorOpen = false;
  displayMessage("DOOR CLOSED");
  Serial.println("Door closed");
}

void verifyPin(String pin) {
  HTTPClient http;
  String url = String(BASE_URL) + "/verificar";
  
  displayMessage("Verifying", "Please wait...");
  Serial.println("Verifying PIN: " + pin);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  JsonDocument doc;
  doc["entrada"] = pin;
  doc["tipo"] = "pin";
  
  String jsonString;
  serializeJson(doc, jsonString);

  int httpCode = http.POST(jsonString);
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
    
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error) {
      bool authorized = responseDoc["autorizado"];
      if (authorized) {
        openDoor();
      } else {
        displayMessage("ACCESS", "DENIED");
        Serial.println("Access denied");
        delay(2000);
      }
    } else {
      displayMessage("ERROR", "Invalid response");
      Serial.println("Failed to parse response");
    }
  } else {
    displayMessage("ERROR", "Connection failed");
    Serial.println("Connection failed");
  }
  
  http.end();
}

bool checkApiStatus() {
  HTTPClient http;
  String url = String(BASE_URL) + "/status";
  
  displayMessage("Checking", "API Status...");
  
  http.begin(url);
  http.addHeader("X-API-Key", API_KEY);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument statusDoc;
    DeserializationError error = deserializeJson(statusDoc, response);
    
    if (!error && statusDoc["status"] == "online") {
      displayMessage("API Online", "Ready!");
      Serial.println("API is online");
      http.end();
      return true;
    }
  }
  
  displayMessage("API Offline", "Press 1 to retry");
  Serial.println("API Status check failed: " + String(httpCode));
  http.end();
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-S3 Project...");

  // Initialize display first for status messages
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Check API Status until success
  bool apiReady = false;
  while (!apiReady) {
    apiReady = checkApiStatus();
    if (!apiReady) {
      while (true) {
        char key = keypad.getKey();
        if (key == '1') {
          break;
        }
        delay(100);
      }
    }
  }

  // Continue with normal setup
  myServo.attach(servoPin);
  myServo.write(0);  // Start in closed position
  displayMessage("Ready", "Enter PIN");
}

// ===== Main Loop =====
void loop() {
  char key = keypad.getKey();
  
  if (key) {
    if (key >= '0' && key <= '9' && currentPin.length() < 5) {
      currentPin += key;
      displayMessage("PIN:", currentPin.c_str());
      Serial.print("*");
    }
    else if (key == '*') {
      currentPin = "";
      displayMessage("PIN CLEARED");
      Serial.println("\nPIN cleared");
      delay(1000);
      displayMessage("Ready", "Enter PIN");
    }
    else if (key == '#' && currentPin.length() == 5) {
      verifyPin(currentPin);
      currentPin = "";
      displayMessage("Ready", "Enter PIN");
    }
    else if (key == '#') {
      displayMessage("ERROR", "PIN must be 5 digits");
      Serial.println("\nPIN must be 5 digits");
      delay(2000);
      displayMessage("Ready", "Enter PIN");
    }
  }
  
  delay(10);
}

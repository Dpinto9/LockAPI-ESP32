#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Index.h>

// ===== Configurações Constantes =====
const char* BASE_URL = "https://api-lock-service-1046300342556.us-central1.run.app";
const char* API_KEY = "api123";
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* LOCALHOST_URL = "http://localhost:8180";

// ===== Configurações de Hardware =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_SDA 17
#define I2C_SCL 18

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {11, 12, 13, 14};
byte colPins[COLS] = {6, 7, 8, 9};

const int servoPin = 10;
const int WEB_SERVER_PORT = 80;

// ===== Objetos Globais =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo myServo;
WebServer server(WEB_SERVER_PORT);

// ===== Variáveis de Estado =====
struct AuthStatus {
  bool is2faActive;
};

String currentPin = "";
bool doorOpen = false;
bool isQRMode = false;
bool serverRunning = false;

// ===== Funções =====
void connectToWiFi();
void displayMessage(const char* line1, const char* line2 = "", const char* line3 = "");
void displayLoading(const char* message);
void openDoor();
void verify2FA(String pin, String qrcode);
void verifyPin(String pin);
void verifyQR(String qrcode);
void processVerificationResponse(int httpCode, HTTPClient* http);
String generateAsterisks(int length);
bool checkApiStatus();
AuthStatus checkAuthStatus();
void handleRoot();
void handleQR();
void handleKeypadInput(char key);
void setupAuthMode();
void toggleServerMode();
void returnToPinMode();

// ===== Implementação das Funções =====
String generateAsterisks(int length) {
  String result = "";
  for (int i = 0; i < length; i++) {
    result += "*";
  }
  return result;
}

AuthStatus checkAuthStatus() {
  Serial.println("\n=== Checking Auth Status ===");
  HTTPClient http;
  String url = String(BASE_URL) + "/2factor/status";
  
  Serial.println("Requesting: " + url);
  http.begin(url);
  http.addHeader("X-API-Key", API_KEY);
  
  AuthStatus status = {false};
  
  int httpCode = http.GET();
  Serial.println("Response code: " + String(httpCode));
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      bool pin_enabled = doc["pin_enabled"].as<bool>();
      bool twofa_enabled = doc["2fa_enabled"].as<bool>();
      String pin_status = doc["status"]["pin"].as<String>();
      String twofa_status = doc["status"]["2fa"].as<String>();
      
      // Only consider active if enabled and status is "active"
      bool pin_active = pin_enabled && (pin_status == "active");
      bool twofa_active = twofa_enabled && (twofa_status == "active");
      
      // Changed logic: PIN mode is active only if PIN is active and 2FA is not
      status.is2faActive = twofa_active;
      
      Serial.println("PIN enabled: " + String(pin_enabled) + ", status: " + pin_status);
      Serial.println("2FA enabled: " + String(twofa_enabled) + ", status: " + twofa_status);
      Serial.println("Final 2FA Status: " + String(status.is2faActive ? "Active" : "Inactive"));
    }
  }
  
  http.end();
  return status;
}

void connectToWiFi() {
  displayMessage("== WiFi ==", "Connecting to", ssid);
  Serial.println("\n=== WiFi Connection ===");
  Serial.println("Connecting to network: " + String(ssid));
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    displayLoading("Connecting to WiFi...");
    Serial.println("Connection attempt " + String(attempts + 1) + " of 20");
    attempts++;
    
    if(attempts > 20) { 
      displayMessage("= ERROR =", "WiFi Connection", "Failed!");
      Serial.println("=== Connection Failed ===");
      Serial.println("Maximum attempts reached");
      delay(2000);
      ESP.restart();
    }
  }

  String ipAddress = "IP: " + WiFi.localIP().toString();
  displayMessage("== WiFi ==", "Connected!", ipAddress.c_str());
  Serial.println("=== Connection Success ===");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  delay(2000);
}

void displayMessage(const char* line1, const char* line2, const char* line3) {
  display.clearDisplay();
  
  int16_t x1, y1;
  uint16_t w, h;
  
  // Draw top border
  display.drawLine(0, 0, SCREEN_WIDTH-1, 0, SSD1306_WHITE);
  
  // Title with larger text
  display.setTextSize(2);
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 4);  
  display.println(line1);
  
  // Draw separator line
  display.drawLine(0, 19, SCREEN_WIDTH-1, 19, SSD1306_WHITE);
  
  // Smaller text for additional lines
  display.setTextSize(1);
  if (strlen(line2) > 0) {
    display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 24);  
    display.println(line2);
  }
  
  if (strlen(line3) > 0) {
    display.getTextBounds(line3, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 36);  
    display.println(line3);
  }
  
  // Draw bottom border
  display.drawLine(0, SCREEN_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SSD1306_WHITE);
  
  display.display();
}

void displayLoading(const char* message) {
  static int position = 0;
  const int centerX = SCREEN_WIDTH / 2;
  const int centerY = 45;  
  const int radius = 8;    
  const int numDots = 12;  
  
  display.clearDisplay();
  
  // Draw top border
  display.drawLine(0, 0, SCREEN_WIDTH-1, 0, SSD1306_WHITE);
  
  // Title with larger text
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  const char* title = "= AUTH ="; 
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 4);
  display.println(title);
  
  // Draw separator line
  display.drawLine(0, 19, SCREEN_WIDTH-1, 19, SSD1306_WHITE);
  
  // Message in middle section
  display.setTextSize(1);
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 24);
  display.println(message);
  
  // Loading animation in bottom section
  for(int i = 0; i < numDots; i++) {
    int angle = (i * (360 / numDots) + position) % 360;
    float rad = angle * PI / 180;
    int x1 = centerX + cos(rad) * radius;
    int y1 = centerY + sin(rad) * radius;
    
    int dotSize = (i == position / (360 / numDots) % numDots) ? 2 : 1;
    
    if (dotSize == 2) {
      display.fillCircle(x1, y1, dotSize, SSD1306_WHITE);
    } else {
      display.drawPixel(x1, y1, SSD1306_WHITE);
    }
  }
  
  // Draw bottom border
  display.drawLine(0, SCREEN_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SSD1306_WHITE);
  
  display.display();
  position = (position + 15) % 360;
  delay(50);
}


// Lógica de verificação
void verifyAuth(const String& endpoint, const JsonDocument& payload) {
  HTTPClient http;
  String url = String(BASE_URL) + endpoint;
  
  Serial.println("[POST] " + url);
  String jsonString;
  serializeJson(payload, jsonString);
  Serial.println("Request Body: " + jsonString);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int httpCode = http.POST(jsonString);
  
  while (http.getSize() <= 0) { 
    displayLoading("Verifying credentials...");
  }

  processVerificationResponse(httpCode, &http);
  http.end();
}

void verify2FA(String pin, String qrcode) {
  JsonDocument doc;
  doc["pin"] = pin;
  doc["qr"] = qrcode;
  verifyAuth("/verificar-2fa", doc);
}

void verifyPin(String pin) {
  JsonDocument doc;
  doc["entrada"] = pin;
  doc["tipo"] = "pin";
  verifyAuth("/verificar", doc);
}

void verifyQR(String qrcode) {
  JsonDocument doc;
  doc["entrada"] = qrcode;
  doc["tipo"] = "qr";
  verifyAuth("/verificar", doc);
}

void processVerificationResponse(int httpCode, HTTPClient* http) {
  if (httpCode > 0) {
    String response = http->getString();
    Serial.println("\n=== Verification Response ===");
    Serial.println("Response: " + response);
    
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error) {
      bool authorized = responseDoc["autorizado"];
      if (authorized) {
        Serial.println("=== Access Granted ===");
        openDoor();
      } else {
        unsigned long startTime = millis();
        while (millis() - startTime < 9000) {
          displayLoading("DENIED!");
        }
        Serial.println("=== Access Denied ===");
        Serial.println("Invalid credentials");
        returnToPinMode();
      }
    } else {
      unsigned long startTime = millis();
      while (millis() - startTime < 9000) {
        displayLoading("Invalid Response!");
      }
      Serial.println("=== Parse Error ===");
      Serial.println("Failed to parse API response");
      returnToPinMode();
    }
  } else {
    unsigned long startTime = millis();
    while (millis() - startTime < 15000) {
      displayLoading("Connection Lost!");
    }
    Serial.println("=== Connection Error ===");
    Serial.println("HTTP Code: " + String(httpCode));
    returnToPinMode();
  }
}


// Verifica o status da API
bool checkApiStatus() {
  Serial.println("\n=== API Status Check ===");
  HTTPClient http;
  String url = String(BASE_URL) + "/status";
  
  http.begin(url); 
  http.addHeader("X-API-Key", API_KEY);
  
  int httpCode = http.GET();
  Serial.println("Checking endpoint: " + url);
  
  while (http.getSize() <= 0) {
    displayLoading("Checking status...");
  }
  
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument statusDoc;
    DeserializationError error = deserializeJson(statusDoc, response);
    
    if (!error && statusDoc["status"] == "online") {
      displayMessage("== API ==", "Online & Ready!", "System active");
      Serial.println("=== API Online ===");
      Serial.println("Status: Ready for operations");
      http.end();
      return true;
    }
  }
  
  displayMessage("== API ==", "Offline", "Press 1 to retry");
  Serial.println("=== API Error ===");
  Serial.println("Status Code: " + String(httpCode));
  http.end();
  return false;
}

void toggleServerMode() {
  if (!serverRunning) {
    server.begin();
    serverRunning = true;
    isQRMode = true;
    displayMessage("=== QR ===", "Scan at:", WiFi.localIP().toString().c_str());
    Serial.println("\n=== QR Server Started ===");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    server.stop();
    serverRunning = false;
    isQRMode = false;
    displayMessage("=== QR ===", "Server Stopped", "Press A to start");
    Serial.println("\n=== QR Server Stopped ===");
  }
}


// Lógica do Teclado
void handleKeypadInput(char key) {
  Serial.println("\n=== Keypad Input: " + String(key) + " ===");

  switch (key) {
    case 'A':
      {
        displayLoading("Checking 2FA ...");
        AuthStatus authStatus = checkAuthStatus();
        if (authStatus.is2faActive && currentPin.length() == 0) {
          Serial.println("=== 2FA Error ===\nPIN required before QR scan");
          displayMessage("== 2FA ==", "Enter PIN first", "Then scan QR");
          delay(2000);
          displayMessage("== PIN ==", "Enter code", "2FA Active");
          return;
        }
        toggleServerMode();
      }
      break;

    case 'B':
      returnToPinMode();
      break;

    case '#':
      if (!isQRMode && currentPin.length() > 0) {
        Serial.println("=== PIN Verification ===\nLength: " + String(currentPin.length()));
        displayLoading("Checking 2FA ...");
        AuthStatus authStatus = checkAuthStatus();
        if (authStatus.is2faActive) {
          displayMessage("== 2FA ==", "PIN Accepted", "Scan QR now");
          // Store PIN and start QR server
          server.begin();
          serverRunning = true;
          isQRMode = true;
          delay(1000);
          displayMessage("=== QR ===", "Scan at:", WiFi.localIP().toString().c_str());
          Serial.println("\n=== QR Server Started ===");
          Serial.println("IP: " + WiFi.localIP().toString());
        } else {
          verifyPin(currentPin);
          currentPin = "";
        }
      }
      break;

    case '*':
      if (!isQRMode) {
        Serial.println("=== PIN Reset ===");
        currentPin = "";
        displayMessage("== PIN ==", "Code Cleared", "Enter new PIN");
      }
      break;

    default:
      if (!isQRMode && key >= '0' && key <= '9' && currentPin.length() < 6) {
        currentPin += key;
        String asterisks = generateAsterisks(currentPin.length());
        Serial.println("=== PIN Input ===\nLength: " + String(currentPin.length()));
        displayMessage("== PIN ==", asterisks.c_str(), "Enter code");
      }
      break;
  }
}

void returnToPinMode() {
  if (serverRunning) {
    server.stop();
    serverRunning = false;
  }
  isQRMode = false;
  currentPin = "";
  AuthStatus authStatus = checkAuthStatus();
  if (authStatus.is2faActive) {
    displayMessage("== 2FA ==", "Enter PIN first", "Then scan QR");
  } else {
    displayMessage("== PIN ==", "Enter code", "Or press A for QR");
  }
}

void openDoor() {
  myServo.write(180);
  doorOpen = true;
  displayMessage("= DOOR =", "ACCESS GRANTED", "Closing in 5s");
  Serial.println("\n=== Door Control ===\nStatus: Opening");
  delay(5000);
  
  myServo.write(0);
  doorOpen = false;
  displayMessage("= DOOR =", "Secured", "Thank you!");
  Serial.println("Status: Closed and locked");
  delay(2000);

  currentPin = "";
  handleKeypadInput('B');
}


// Função para lidar com a página inicial
void handleRoot() {
  server.send(200, "text/html", WEBPAGE_HTML);
}

void handleQR() {
  if (server.hasArg("plain")) {
    String postBody = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, postBody);
    
    if (!error) {
      String qrcode = doc["qrcode"].as<String>();
      if (qrcode.length() > 0) {
        Serial.println("\n=== QR Code Received ===");
        AuthStatus authStatus = checkAuthStatus();
        
        if (authStatus.is2faActive) {
          if (currentPin.length() > 0) {
            verify2FA(currentPin, qrcode);
            currentPin = "";
            returnToPinMode();
            displayMessage("== 2FA ==", "Verification", "Complete!");
          } else {
            displayMessage("= ERROR =", "PIN Required", "Press B for PIN");
          }
        } else {
          verifyQR(qrcode);
          returnToPinMode();
        }
      }
    }
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}


void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Lock System Starting ===");

  // Inicializar o display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("=== Display Error ===");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  displayMessage("= LOCK =", "Starting up", "Please wait");

  // Conectar ao Wi-Fi
  connectToWiFi();

  // Verificar status da API até que esteja online
  bool apiReady = false;
  while (!apiReady) {
    Serial.println("\n=== Checking API Status ===");
    apiReady = checkApiStatus();
    if (!apiReady) {
      Serial.println("=== API Offline ===");
      displayMessage("= API =", "Offline", "Press 1 retry");
      while (true) {
        char key = keypad.getKey();
        if (key == '1') break;
        delay(100);
      }
    }
  }

  // Inicializar rotas do servidor web
  server.on("/", handleRoot);
  server.on("/qr", HTTP_POST, handleQR);

  // Inicializar o servo
  myServo.attach(servoPin);
  myServo.write(0);
  Serial.println("=== Servo Initialized ===");

  // Verificar status inicial de autenticação
  AuthStatus authStatus = checkAuthStatus();
  if (authStatus.is2faActive) {
    displayMessage("= 2FA =", "Enter PIN", "Then scan QR");
  } else {
    displayMessage("= LOCK =", "Enter PIN or", "Press A: QR");
  }
  
  Serial.println("=== System Ready ===");
}

void loop() {
  if (serverRunning) {
    server.handleClient();
  }

  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
}
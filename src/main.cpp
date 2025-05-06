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

// ===== Configurações Constantes =====
const char* BASE_URL = "https://api-lock-service-1046300342556.us-central1.run.app";
const char* API_KEY = "api123";
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* LOCALHOST_URL = "http://localhost:8180";

// Certificado CA para conexões HTTPS
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n" \
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n" \
"MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n" \
"MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n" \
"Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n" \
"A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n" \
"27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n" \
"Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n" \
"TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n" \
"qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n" \
"szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n" \
"Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n" \
"MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n" \
"wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n" \
"aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n" \
"VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n" \
"AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n" \
"FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n" \
"C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n" \
"QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n" \
"h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n" \
"7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n" \
"ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n" \
"MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n" \
"Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n" \
"6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n" \
"0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n" \
"2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n" \
"bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n" \
"-----END CERTIFICATE-----\n";

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

// ===== Protótipos de Função =====
void connectToWiFi();
void displayMessage(const char* line1, const char* line2 = "", const char* line3 = "");
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

AuthStatus checkAuthStatus() {
  Serial.println("\n=== Checking Auth Status ===");
  HTTPClient http;
  String url = String(BASE_URL) + "/2factor/status";
  
  Serial.println("Requesting: " + url);
  http.begin(url, root_ca);
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
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Tentando conectar...");
  }

  Serial.println("Conectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void displayMessage(const char* line1, const char* line2, const char* line3) {
  display.clearDisplay();
  
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(line2);
  
  if (line3[0] != '\0') {
    display.setCursor(0, 32); 
    display.println(line3);
  }
  
  display.display();
}

void openDoor() {
  myServo.write(180);  // Open position
  doorOpen = true;
  displayMessage("DOOR", "OPEN", "Closing in 5s");
  Serial.println("Door opened");
  delay(5000);
  myServo.write(0);    // Closed position
  doorOpen = false;
  displayMessage("DOOR", "CLOSED");
  Serial.println("Door closed");
  delay(2000);

  currentPin = "";  
  handleKeypadInput('B');  
}

// In verifyAuth function
void verifyAuth(const String& endpoint, const JsonDocument& payload) {
  HTTPClient http;
  String url = String(BASE_URL) + endpoint;
  
  Serial.println("[POST] " + url);
  String jsonString;
  serializeJson(payload, jsonString);
  Serial.println("Request Body: " + jsonString);
  
  displayMessage("Verifying", "Please wait...");
  
  http.begin(url, root_ca);  // Using the new root certificate
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int httpCode = http.POST(jsonString);
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
}

// In checkApiStatus function
bool checkApiStatus() {
  HTTPClient http;
  String url = String(BASE_URL) + "/status";
  
  displayMessage("Checking", "API Status...");
  
  http.begin(url, root_ca);  // Using the new root certificate
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

void toggleServerMode() {
  if (!serverRunning) {
    server.begin();
    serverRunning = true;
    isQRMode = true;
    displayMessage("QR Mode", WiFi.localIP().toString().c_str(), "Scan at:");
    Serial.println("Server started for QR reading");
  } else {
    server.stop();
    serverRunning = false;
    isQRMode = false;
    displayMessage("Server OFF", "Press A to start");
    Serial.println("Server stopped");
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
    displayMessage("2FA Active", "Enter PIN first");
  } else {
    displayMessage("PIN Mode", "Enter PIN");
  }
}

const char* WEBPAGE_HTML = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>QR Code Scanner</title>
      <style>
          body {
              font-family: Arial, sans-serif;
              display: flex;
              flex-direction: column;
              align-items: center;
              background-color: #f0f0f0;
              margin: 0;
              padding: 20px;
          }
          .container {
              max-width: 800px;
              width: 100%;
              text-align: center;
          }
          #video {
              width: 100%;
              max-width: 640px;
              margin: 20px 0;
              border-radius: 8px;
              box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }
          #output {
              margin: 20px 0;
              padding: 15px;
              border-radius: 8px;
              background-color: white;
              box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }
          .error {
              color: red;
              margin: 10px 0;
          }
      </style>
  </head>
  <body>
      <div class="container">
          <h1>QR Code Scanner</h1>
          <video id="video" playsinline></video>
          <div id="output">No QR code detected</div>
      </div>
      <script src="https://cdn.jsdelivr.net/npm/jsqr@1.4.0/dist/jsQR.js"></script>
      <script>
      let video = document.getElementById('video');
      let canvasElement = document.createElement('canvas');
      let canvas = canvasElement.getContext('2d');
      let outputData = document.getElementById('output');
  
      let scanning = false;
      let videoStream = null;
  
      document.addEventListener('DOMContentLoaded', startScanning);
  
      async function startScanning() {
          try {
              videoStream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: "environment" } });
              video.srcObject = videoStream;
              video.play();
              scanning = true;
              requestAnimationFrame(tick);
          } catch (err) {
              console.error('Error accessing camera:', err);
              outputData.textContent = `Error accessing camera: ${err.message}`;
              outputData.classList.add('error');
          }
      }
  
      function tick() {
          if (video.readyState === video.HAVE_ENOUGH_DATA && scanning) {
              canvasElement.height = video.videoHeight;
              canvasElement.width = video.videoWidth;
              canvas.drawImage(video, 0, 0, canvasElement.width, canvasElement.height);
              let imageData = canvas.getImageData(0, 0, canvasElement.width, canvasElement.height);
              
              try {
                  let code = jsQR(imageData.data, imageData.width, imageData.height);
                  if (code) {
                      outputData.textContent = `Found QR code: ${code.data}`;
                      fetch('/qr', {
                          method: 'POST',
                          headers: {
                              'Content-Type': 'application/json',
                          },
                          body: JSON.stringify({ qrcode: code.data })
                      });
                  }
              } catch (e) {
                  console.error('QR detection error:', e);
              }
              
              requestAnimationFrame(tick);
          } else if (scanning) {
              requestAnimationFrame(tick);
          }
      }
      </script>
  </body>
  </html>
  )rawliteral";

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
        Serial.println("QRCode received: " + qrcode);
        AuthStatus authStatus = checkAuthStatus();
        
        if (authStatus.is2faActive) {
          if (currentPin.length() > 0) {
            verify2FA(currentPin, qrcode);
            currentPin = "";
            returnToPinMode();
            displayMessage("Verification", "Complete");
          } else {
            displayMessage("Error", "Enter PIN first", "Press B for PIN");
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

String generateAsterisks(int length) {
  String result = "";
  for (int i = 0; i < length; i++) {
    result += "*";
  }
  return result;
}

void handleKeypadInput(char key) {
  Serial.println("\n=== Keypad Input: " + String(key) + " ===");

  switch (key) {
    case 'A':
      {
        AuthStatus authStatus = checkAuthStatus(); // Only check when switching modes
        if (authStatus.is2faActive && currentPin.length() == 0) {
          Serial.println("Error: PIN required for 2FA before using A");
          displayMessage("2FA Error", "Enter PIN first", "2FA is active");
          delay(2000);
          displayMessage("Enter PIN", "", "2FA Active");
          return;
        }
        toggleServerMode();
      }
      break;

    case 'B':
      returnToPinMode();  // returnToPinMode already checks auth status
      break;

    case '#':
      if (!isQRMode && currentPin.length() > 0) {
        Serial.println("Verifying PIN: " + String(currentPin.length()) + " digits");
        displayMessage("Checking", "API Status...");
        AuthStatus authStatus = checkAuthStatus(); // Only check when verifying PIN
        if (authStatus.is2faActive) {
          displayMessage("PIN OK", "Press A for QR", "2FA Required");
        } else {
          verifyPin(currentPin);
          currentPin = "";
        }
      }
      break;

    case '*':
      if (!isQRMode) {
        Serial.println("PIN Cleared");
        currentPin = "";
        displayMessage("PIN Cleared", "Enter PIN");
      }
      break;

    default:
      if (!isQRMode && key >= '0' && key <= '9' && currentPin.length() < 6) {
        currentPin += key;
        String asterisks = generateAsterisks(currentPin.length());
        String keyMessage = "Key: ";
        keyMessage += key;
        Serial.println("PIN Input: " + asterisks + " (Key pressed: " + String(key) + ")");
        displayMessage("Enter PIN", asterisks.c_str(), keyMessage.c_str());
        delay(500);
        displayMessage("Enter PIN", asterisks.c_str());
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando projeto ESP32-S3...");

  // Inicializar o display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do display SSD1306"));
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Conectar ao Wi-Fi
  connectToWiFi();

  // Verificar status da API até que esteja online
  bool apiReady = false;
  while (!apiReady) {
    Serial.println("Verificando status da API...");
    apiReady = checkApiStatus();
    if (!apiReady) {
      Serial.println("API Offline. Pressione '1' para tentar novamente.");
      while (true) {
        char key = keypad.getKey();
        if (key == '1') {
          break;
        }
        delay(100);
      }
    }
  }

  // Inicializar rotas do servidor web
  server.on("/", handleRoot);
  server.on("/qr", HTTP_POST, handleQR);

  // Inicializar o servo
  myServo.attach(servoPin);
  myServo.write(0);  // Começar na posição fechada

  // Verificar status inicial de autenticação e exibir mensagem apropriada
  AuthStatus authStatus = checkAuthStatus();
  if (authStatus.is2faActive) {
    displayMessage("2FA Active", "Enter PIN first");
  } else {
    displayMessage("Ready", "Enter PIN or", "Press A for QR");
  }
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
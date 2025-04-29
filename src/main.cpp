#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// ===== OLED Setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin (not used)

#define I2C_SDA 17
#define I2C_SCL 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-S3 Project...");

  // Start I2C for OLED with custom SDA/SCL pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is the default I2C address
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // freeze
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Ready!");
  display.display();

  // Initialize Servo
  myServo.attach(servoPin);
  myServo.write(90); // Start centered
}

// ===== Main Loop =====
void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);

    // Move Servo based on key
    switch (key) {
      case '1':
        myServo.write(0);
        break;
      case '2':
        myServo.write(45);
        break;
      case '3':
        myServo.write(90);
        break;
      case '4':
        myServo.write(135);
        break;
      case '5':
        myServo.write(180);
        break;
      default:
        // For other keys, do not move the servo
        break;
    }

    // Update OLED display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print("Key: ");
    display.println(key);
    display.display();
  }

  delay(10); // small delay for stability
}

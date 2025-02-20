#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>

const int ledPin1 = 14;
const int ledPin2 = 12;
const int buzzerPin = 16;

// Create an instance of the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 columns, 2 rows

// TDS Sensor pin
#define TDS_PIN A0 // Analog pin for the TDS sensor

// Calibration constants
const float VOLTAGE_REF = 3.3; // ESP8266 ADC reference voltage
const float TDS_FACTOR = 0.5; // Adjust based on calibration (try tuning this)

// Detection thresholds
const float SALINE_THRESHOLD = 4800;      // TDS above this is high-purity saline
const float CONTAMINATED_THRESHOLD = 3900; // TDS above this is contaminated saline
const float NORMAL_THRESHOLD = 335;     // TDS above this is water
const float MIN_VALID_VOLTAGE = 0.1;      // Minimum voltage to detect input

// Number of samples for averaging
const int NUM_SAMPLES = 50;

// Function to calculate TDS value
float getTDSValue() {
  int rawSum = 0;

  // Take multiple samples for averaging
  for (int i = 0; i < NUM_SAMPLES; i++) {
    rawSum += analogRead(TDS_PIN);
    delay(1); // Small delay between samples
  }

  int rawAverage = rawSum / NUM_SAMPLES;               // Average raw ADC value
  float voltage = (rawAverage / 1024.0) * VOLTAGE_REF; // Convert ADC value to voltage

  // Debug: Print raw ADC value and voltage
  Serial.print("Raw ADC Value: ");
  Serial.println(rawAverage);
  Serial.print("Voltage: ");
  Serial.println(voltage, 3); // Print voltage with 3 decimal places

  // Validate input voltage
  if (voltage < MIN_VALID_VOLTAGE) {
    return -1; // Invalid input or no liquid detected
  }

  float tdsValue = (voltage / TDS_FACTOR) * 1000; // Convert voltage to TDS in ppm
  return tdsValue;
}

void setup() {
  Wire.begin();           // Initialize I2C communication
  lcd.begin(16, 2);       // Initialize the LCD with 16 columns and 2 rows
  lcd.backlight();        // Turn on the LCD backlight

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);
  Serial.println("Saline Detection Started...");
}

void loop() {
  float tdsValue = getTDSValue();

  if (tdsValue == -1) {
    // No valid input detected
    Serial.println("No valid input detected (sensor not connected or dry).");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dry");
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    noTone(buzzerPin);
  } else {
    Serial.print("TDS Value: ");
    Serial.print(tdsValue, 2); // Print with 2 decimal places
    Serial.println(" ppm");

    // Classify liquid based on TDS value
    if (tdsValue >= SALINE_THRESHOLD) {
      Serial.println("Liquid Detected: Pure Saline");
      digitalWrite(ledPin2, LOW); // LED2 OFF
      digitalWrite(ledPin1, LOW); // LED1 OFF
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pure Saline");
      noTone(buzzerPin); // Buzzer OFF
    } else if (tdsValue >= CONTAMINATED_THRESHOLD && tdsValue < SALINE_THRESHOLD) {
      Serial.println("Liquid Detected: Contaminated");
      digitalWrite(ledPin1, HIGH); // LED1 ON
      digitalWrite(ledPin2, LOW);  // LED2 OFF
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Contaminated");
      tone(buzzerPin, 500);
      delay(1000);
      noTone(buzzerPin);           // Buzzer OFF
    } else if (tdsValue >= NORMAL_THRESHOLD && tdsValue < CONTAMINATED_THRESHOLD) {
      Serial.println("Liquid Detected: Water");
      digitalWrite(ledPin1, LOW);  // LED1 OFF
      digitalWrite(ledPin2, HIGH); // LED2 ON
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water");
      tone(buzzerPin, 500);        // Buzzer ON
      delay(1000);
      noTone(buzzerPin);           // Buzzer OFF
    } else {
      Serial.println("Liquid Detected: Unknown substance (Below water threshold)");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unknown");
      digitalWrite(ledPin1, LOW);
      digitalWrite(ledPin2, LOW);
      noTone(buzzerPin);
    }
  }

  delay(2000); // Wait for 2 seconds before the next reading
}

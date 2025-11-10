#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ultrasonic.h>

// ===== WiFi credentials =====
const char* ssid = "POCO F3";
const char* password = "Ansontay7";

// ===== Google Script URL =====
const char* scriptURL = "https://script.google.com/macros/s/AKfycbyxVwynTrWX3fdPerYJBKoatqPBzLYWMEcITHwqCwC2wIn2bl2_lQkBGFlR35IaMISy/exec";

// ===== BME280 setup =====
Adafruit_BME280 bme;  // I2C interface

// ===== Ultrasonic setup =====
#define TRIG_PIN 13
#define ECHO_PIN 12
Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

// ===== Timing =====
const unsigned long sendInterval = 600000; // 10 minutes (600,000 ms)
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 BME280 + Ultrasonic Data Logger");

  // Initialize BME280
  if (!bme.begin(0x76)) {  // I2C address can be 0x76 or 0x77
    Serial.println("Could not find BME280 sensor!");
    while (1);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= sendInterval) {
    previousMillis = currentMillis;

    // ===== Read sensor data =====
    float temperature = bme.readTemperature(); // °C
    float humidity = bme.readHumidity();       // %
    long distance = ultrasonic.read();         // cm

    if (isnan(temperature) || isnan(humidity) || distance <= 0) {
      Serial.println("Failed to read from sensors!");
      return;
    }

    // Print readings
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Send to Google Sheets
    sendDataToGoogleSheets(temperature, humidity, distance);
  }
}

void sendDataToGoogleSheets(float temperature, float humidity, long distance) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  http.begin(scriptURL);
  http.addHeader("Content-Type", "application/json");

  // Create JSON data
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["distance"] = distance;

  String jsonString;
  serializeJson(doc, jsonString);

  // Send HTTP POST request
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

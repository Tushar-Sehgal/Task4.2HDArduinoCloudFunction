#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "secret.h"

// Constants for the Firebase host and HTTPS port
const char* FIREBASE_HOST = "traffic-light-sim-default-rtdb.asia-southeast1.firebasedatabase.app";
const int HTTPS_PORT = 443;

// LED pin assignments on the Arduino board
const int LED_RED = 2;
const int LED_GREEN = 3;
const int LED_BLUE = 4;

// Setting up the WiFi and HTTP client objects
WiFiSSLClient wifiClient;
HttpClient httpClient(wifiClient, FIREBASE_HOST, HTTPS_PORT);

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  connectWifi();
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
}

void loop() {
  // Updates traffic status based on Firebase data
  if (updateTrafficStatus()) {
    Serial.println("Traffic status updated");
  } else {
    Serial.println("Status update failed, try using the serial monitor maybe");
  }
  
  // Toggles LEDs based on data that is available in the serial monitor to read
  if (Serial.available()) {
    String color = Serial.readStringUntil('\n');
    color.trim();
    color.toLowerCase();
    manualOverride(color);
  }

  // 1 second delay between 2 requests
  delay(1000);
}

// Function to connect to WiFi
void connectWifi() {
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(SECRET_SSID, SECRET_PASS);
      Serial.print(".");
      delay(10000);     
    }
  }
  Serial.println("\nWiFi connected!");
}

// Function to toggle LEDs based on the input color
void manualOverride(String color) {
    int ledPin = -1;
    String path;

    // First, determine which LED is being toggled and prepare the path
    if (color == "red") {
        ledPin = LED_RED;
        path = "/LEDcolor.json";
    } else if (color == "green") {
        ledPin = LED_GREEN;
        path = "/LEDcolor.json";
    } else if (color == "blue") {
        ledPin = LED_BLUE;
        path = "/LEDcolor.json";
    }

    if (ledPin != -1) {
        // Turn all LEDs off first
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);

        // Then turn only the selected LED on
        digitalWrite(ledPin, HIGH);

        // Update all LED states in Firebase to ensure only the selected one is on
        DynamicJsonDocument doc(1024);
        doc["Red"] = (ledPin == LED_RED);
        doc["Green"] = (ledPin == LED_GREEN);
        doc["Blue"] = (ledPin == LED_BLUE);
        
        // Serialize JSON to send to Firebase
        String output;
        serializeJson(doc, output);

        // Send a PUT request to update all LED states in Firebase
        httpClient.beginRequest();
        httpClient.put(path, "application/json", output);
        httpClient.endRequest();
        
        int statusCode = httpClient.responseStatusCode();
        if (statusCode == 200) {
            Serial.print(color + " LED manually set to ON and updated in Firebase\n");
        } else {
            Serial.print("Failed to update Firebase. Status code: ");
            Serial.println(statusCode);
        }
    } else {
        Serial.println("Invalid color specified");
    }
}

// Function to update the LED status from Firebase
bool updateTrafficStatus() {
  httpClient.beginRequest();
  httpClient.get("/LEDcolor.json");
  httpClient.endRequest();
  
  int statusCode = httpClient.responseStatusCode();
  if (statusCode == 200) {
      String response = httpClient.responseBody();
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, response);
      
      if (!error) {
          digitalWrite(LED_RED, doc["Red"] ? HIGH : LOW);
          digitalWrite(LED_GREEN, doc["Green"] ? HIGH : LOW);
          digitalWrite(LED_BLUE, doc["Blue"] ? HIGH : LOW);
          return true;
      } else {
          Serial.print("JSON deserialize failed: ");
          Serial.println(error.c_str());
      }
  } else {
      Serial.print("HTTP GET failed, status code = ");
      Serial.println(statusCode);
  }

  Serial.println("Status update failed, try using the serial monitor maybe");
  return false;
}
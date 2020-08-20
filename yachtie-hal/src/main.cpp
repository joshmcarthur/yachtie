#include <Arduino.h>
#include "ArduinoJson.h"
#include "DHT.h"
#include "Servo.h"
#include "SoftwareSerial.h"
#include "OneWire.h"
#include "DallasTemperature.h"

#define DHTPIN 12
#define GPS_RX_PIN 3
#define GPS_TX_PIN 4
#define GPS_SERIAL_BAUD 9600
#define ONEWIRE_BUS_PIN 11
#define RUDDER_SERVO_PIN 10
#define DHTTYPE DHT11

// This is not compatible with Arduino's builtin serial monitor,
// but we want to push JSON as fast as we can.
#define SERIAL_BAUD 250000

DHT dht(DHTPIN, DHTTYPE);
Servo rudderServo;
SoftwareSerial nss(GPS_RX_PIN, GPS_TX_PIN);
OneWire oneWire(ONEWIRE_BUS_PIN);
DallasTemperature oneWireSensors(&oneWire);

void dispatchCommand(const char* type, const char* value) {
  if (strcmp(type, "rudder_absolute") == 0) {
    // Write to servo
    return;
  }

  if (strcmp(type, "rudder_relative") == 0) {    
    // Write to servo, but use current rudder position
    // to calculate the relative change
    return;
  }
}

void sendMessage(const char* type, const char* value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
}

void sendMessage(const char* type, const String value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
}

void sendMessage(const char* type, const float value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
}


void setup()
{
  Serial.begin(SERIAL_BAUD);

  sendMessage("message", "GPS started.");
  nss.begin(GPS_SERIAL_BAUD);

  sendMessage("message", "DHT started.");
  dht.begin();

  sendMessage("message", "Onewire bus started.");
  oneWireSensors.begin();

}

void loop()
{
  if (Serial.available()) {
    const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, Serial);

    if (!err) {
      dispatchCommand(doc["type"], doc["value"].as<char*>());
    } else {
      sendMessage("error", err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (Serial.available() > 0)
        Serial.read();    
    }
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();

  if (!isnan(temperature))
    sendMessage("temperature_celsius", temperature);

  if (!isnan(humidity))
    sendMessage("humidity_rh", humidity);

  String nmea = nss.readStringUntil('\r') + "\n";
  if (nmea.length() > 1) // More than just a newline?
    sendMessage("gps_nmea", nmea);
}
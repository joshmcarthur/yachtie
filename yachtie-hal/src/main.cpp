#include <Arduino.h>
#include "ArduinoJson.h"
#include "DHT.h"
#include "ServoTimer2.h"
#include "AltSoftSerial.h"

#define DHTPIN 12
#define GPS_SERIAL_BAUD 9600
#define RUDDER_SERVO_PIN 2

#define DHTTYPE DHT11
#define RUDDER_RANGE_MIN -90L
#define RUDDER_RANGE_MAX 90L

#define SERIAL_BAUD 115200

DHT dht(DHTPIN, DHTTYPE);
ServoTimer2 rudderServo;
AltSoftSerial gpsSerial; // MUST receive on pin 8

const long SERVO_MIN = 1000L;
const long SERVO_MAX = 2000L;

void sendMessage(const char* type, const char* value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
  Serial.print('\n');
}

int scale(int valueIn, long baseMin, long baseMax, int limitMin, int limitMax) {
  return ((limitMax - limitMin) * (valueIn - baseMin) / (baseMax - baseMin)) + limitMin;
}

// ((2000 - 1000) * (0 - -90) / (90 - -90)) + 1000
// ((1000 * 90) / 180) + 1000

int rudderScale(const int degrees) {
  return scale(degrees, RUDDER_RANGE_MIN, RUDDER_RANGE_MAX, SERVO_MIN, SERVO_MAX);
}

int rotateRudder(int ms) {
  // Attach the servo
  rudderServo.attach(RUDDER_SERVO_PIN);
  delay(100);

  // Rotate the servo
  rudderServo.write(ms);

  delay(100);
  // Detach the servo to reduce jitter
  rudderServo.detach();
}

void rudderServoSelfTest() {
  rotateRudder(1000);
  delay(1000);
  rotateRudder(1500);
  delay(1000);
  rotateRudder(2000);
  delay(1000);
  rotateRudder(1500);
}

// Rudder servo ranges from 1000 to 2000, with 1500 being center
// We accept rudder values from -90 (hard to port) to 90 (hard to starboard)
void dispatchCommand(const char* type, const int value) {
  if (strcmp(type, "rudder_absolute") == 0) {
    // Write to servo
    int absoluteDegree = (int)value;
    int absoluteMs = rudderScale(absoluteDegree);
    rotateRudder(absoluteMs);

    sendMessage(type, absoluteDegree);
    return;
  }
}

void sendMessage(const char* type, const String value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
  Serial.print('\n');
}

void sendMessage(const char* type, const float value) {
  const int kvCapacity = 512; // bytes
  StaticJsonDocument<kvCapacity> kvMessage;

  kvMessage["type"] = type;
  kvMessage["value"] = value;

  serializeJson(kvMessage, Serial);
  Serial.print('\n');
}

void setup()
{
  Serial.begin(SERIAL_BAUD);

  gpsSerial.begin(GPS_SERIAL_BAUD);
  sendMessage("message", "GPS started.");

  dht.begin();
  sendMessage("message", "DHT started.");

  rudderServoSelfTest();
  sendMessage("message", "Rudder servo attached.");
}

void loop()
{
  if (Serial.available()) {
    const size_t capacity = 256;
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, Serial);

    if (!err) {
      dispatchCommand(doc["type"], doc["value"]);
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

  String nmea = gpsSerial.readStringUntil('\r\n') + "\n";
  if (nmea.length() > 1) // More than just a newline?
    sendMessage("gps_nmea", nmea);
}
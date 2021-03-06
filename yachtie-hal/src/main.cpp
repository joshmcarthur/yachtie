#include <Arduino.h>
#include "ArduinoJson.h"
#include "DHT.h"
#include "ServoTimer2.h"
#include "AltSoftSerial.h"
#include "TinyGPS++.h"

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
TinyGPSPlus gps;

const long SERVO_MIN = 1000L;
const long SERVO_MAX = 2000L;

float lastTemp = NAN;
float lastHumidity = NAN;

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

  if (!isnan(temperature) && temperature != lastTemp) {
    lastTemp = temperature;
    sendMessage("temperature_celsius", lastTemp);
  }

  if (!isnan(humidity) && humidity != lastHumidity) {
    lastHumidity = humidity;
    sendMessage("humidity_rh", lastHumidity);
  }

  while (gpsSerial.available()) 
    gps.encode(gpsSerial.read());

  if (gps.satellites.isUpdated()) 
    sendMessage("gps_satellites", gps.satellites.value());
  

  if (gps.speed.isUpdated()) 
    sendMessage("gps_speed", gps.speed.knots());
  
  if (gps.course.isUpdated()) 
    sendMessage("gps_course", gps.course.deg());

  if (gps.hdop.isUpdated() && gps.satellites.value() > 0) 
    sendMessage("gps_hdop", gps.hdop.value());

  if (gps.location.isUpdated()) {
    String latlng = "";
    latlng.concat(String(gps.location.lat(), 10));
    latlng.concat(",");
    latlng.concat(String(gps.location.lng(), 10));
    sendMessage("gps_location", latlng);
  } else if (gps.location.isValid()) {
    sendMessage("gps_location_age", gps.location.age());
  }

  delay(250);
}
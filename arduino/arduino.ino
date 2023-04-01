#include <WiFiNINA.h>
#include <Adafruit_Sensor.h>
#include <ArduinoMqttClient.h>

// #define DEBUG

#include "sensor.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"

#define LIGHT_SENSOR_PIN A6
#define SENSOR_READINGS_BUFFER_SIZE 500


Adafruit_BME680 bmeSensor;  // BME sensor over I2C
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long sequence = 0L;  // can be incremented every second for over 130 years
bool connectedToBroker = false;

SensorReadings currentSensorReadings;
SensorReadings sensorReadingsBuffer[SENSOR_READINGS_BUFFER_SIZE];
unsigned int bufferCount = 0;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // builtin LED on indicates the initialization process is still running

  Serial.begin(115200);

#ifdef DEBUG
  while (!Serial)  // If the Arduino is not connected to a PC there is no serial console, so it would be stuck.
    ;
#endif

  if (!initSensor(&bmeSensor)) {
    while (true)
      ;
  }

  if (!checkWiFiModule()) {
    while (true)
      ;
  }

  WiFi.noLowPowerMode();
  WiFi.setHostname(WIFI_HOSTNAME);

  mqttClient.setCleanSession(true);
  mqttClient.setId(MQTT_CLIENT_ID);

  while (!connectWifi()) {
    delay(4000);
  }

  delay(1000);

  while (!connectToMqttBroker(&mqttClient)) {
    delay(4000);
  }

#ifdef DEBUG
  Serial.print("Getting time from WiFi ... ");
#endif

  while (WiFi.getTime() == 0) {
    delay(2000);  // To initially fetch the current time from the network. Would be 0 the first times.
  }

#ifdef DEBUG
  Serial.print("done. Unix time is ");
  Serial.println(WiFi.getTime());
#endif

  connectedToBroker = true;

  digitalWrite(LED_BUILTIN, LOW);  // builtin LED off indicates the initialization process is finished
}

void checkConnectionsAndReconnectIfNecessary() {
  if (connectedToBroker) {
#ifdef DEBUG
    Serial.println("Already connected to broker.");
#endif
    return;
  }

  if (!wifiClient.connected()) {
    Serial.println("ERROR: WiFi connection lost!");

    if (!connectWifi()) {
      Serial.println("Wont reconnect to MQTT broker because WiFi is not connected!");
      return;
    }
  }

  connectedToBroker = connectToMqttBroker(&mqttClient);

#ifdef DEBUG
  if (connectedToBroker) {
    Serial.println("Reconnection to broker successful.");
  } else {
    Serial.println("Reconnection to broker unsuccessful.");
  }
#endif
}

void saveCurrentSensorReadingsToBuffer() {
  if (bufferCount >= SENSOR_READINGS_BUFFER_SIZE) {
    Serial.println("Error: sensor readings buffer is full!");
    return;
  }

  sensorReadingsBuffer[bufferCount] = {
    temperature: currentSensorReadings.temperature,
    pressure: currentSensorReadings.pressure,
    humidity: currentSensorReadings.humidity,
    gasResistance: currentSensorReadings.gasResistance,
    lightIntensity: currentSensorReadings.lightIntensity,
    timestamp: currentSensorReadings.timestamp,
    sequence: currentSensorReadings.sequence,
  };

  bufferCount++;
}

void sendBufferIfItHasElements() {
  if (bufferCount == 0) {
#ifdef DEBUG
    Serial.println("Wont send buffer because bufferCount is zero.");
#endif

    return;
  }

  Serial.print("Buffer contains ");
  Serial.print(bufferCount);
  Serial.println(" sensor readings. Trying to send them ...");

  if (!connectedToBroker) {
    Serial.println("ERROR: Wont send buffer because services are not connected!");
    return;
  }

  unsigned int totalSensorReadingsToSend = bufferCount;

  digitalWrite(LED_BUILTIN, HIGH);

  for (unsigned int i = 0; i < totalSensorReadingsToSend; ++i) {
    delay(50);

    SensorReadings sensorReadingsToSend = sensorReadingsBuffer[totalSensorReadingsToSend - i - 1];

    bool sendSuccess = sendSensorReadingsToMqttBroker(&sensorReadingsToSend, &mqttClient);

    if (!sendSuccess) {
      Serial.println("ERROR: Failed to send sensor readings from buffer!");
      connectedToBroker = false;
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }

    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(totalSensorReadingsToSend);
    Serial.println(" sensor readings sent.");

    bufferCount--;
  }

  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
#ifdef DEBUG
  Serial.println();
#endif

  mqttClient.poll();

  checkConnectionsAndReconnectIfNecessary();

  sendBufferIfItHasElements();

  bool readSuccess = readSensorsOversampled(&currentSensorReadings, sequence, &bmeSensor, LIGHT_SENSOR_PIN);

  if (!readSuccess) {
    Serial.println("ERROR: Failed to perform sensor reading!");
    delay(2000);
    return;
  }

  sequence++;

#ifdef DEBUG
  printSensorReadings(&currentSensorReadings);
#endif

  if (!connectedToBroker) {
    Serial.println("ERROR: Not connected to broker! Will save sensor reading into buffer.");
    saveCurrentSensorReadingsToBuffer();
    return;
  }

#ifdef DEBUG
  Serial.print("Sending sensor readings ... ");
#endif

  bool sendSuccess = sendSensorReadingsToMqttBroker(&currentSensorReadings, &mqttClient);

  if (!sendSuccess) {
    Serial.println("ERROR: Not connected to broker! Will save sensor reading into buffer.");
    saveCurrentSensorReadingsToBuffer();
    connectedToBroker = false;
    return;
  }

#ifdef DEBUG
  Serial.println("done.");
#endif

  // blink every time a sensor reading could be send via the normal way
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
}

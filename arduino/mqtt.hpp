#pragma once

#include <ArduinoMqttClient.h>
#include "sensor.hpp"
#include "secrets.h"


/**
* Initializing MQTT client.
* Connect to MQTT server.
* Returns true on success and false if connecting fails.
*/
bool connectToMqttBroker(MqttClient* mqttClient) {
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.print(MQTT_BROKER);
  Serial.print(" ... ");

#if MQTT_USE_CREDENTIALS
  mqttClient->setUsernamePassword(MQTT_USER, MQTT_PASSWORD);
#endif

  if (!mqttClient->connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient->connectError());

    return false;
  }

  Serial.println("done.");
  return true;
}

bool sendSingleMeasurementMqttMessage(float value, const char* unit, unsigned long sequence, unsigned long timestamp, const char* topic, MqttClient* mqttClient) {
  int beginMessageSuccess = mqttClient->beginMessage(topic);
  if (!beginMessageSuccess) {
    Serial.print("ERROR: Could not begin single measurement MQTT message (sequence=");
    Serial.print(sequence);
    Serial.print(", timestamp=");
    Serial.print(timestamp);
    Serial.println(")");
    return false;
  }

  mqttClient->print("{\"value\": ");
  mqttClient->print(value);
  mqttClient->print(", \"unit\": \"");
  mqttClient->print(unit);
  mqttClient->print("\", \"sequence\": ");
  mqttClient->print(sequence);
  mqttClient->print(", \"timestamp\": ");
  mqttClient->print(timestamp);
  mqttClient->print("}");

  bool status = mqttClient->endMessage();
  return status == 1;
}

bool sendCompleteSensorReadingsMqttMessage(const SensorReadings* sensorReadings, const char* topic, MqttClient* mqttClient) {
  int beginMessageSuccess = mqttClient->beginMessage(topic);
  if (!beginMessageSuccess) {
    Serial.print("ERROR: Could not begin complete sensor readings MQTT message (sequence=");
    Serial.print(sensorReadings->sequence);
    Serial.print(", timestamp=");
    Serial.print(sensorReadings->timestamp);
    Serial.println(")");
    return false;
  }

  mqttClient->print("{\"tempi\": ");
  mqttClient->print(sensorReadings->temperature);
  mqttClient->print(", \"pressure\": ");
  mqttClient->print(sensorReadings->pressure);
  mqttClient->print(", \"humidity\": ");
  mqttClient->print(sensorReadings->humidity);
  mqttClient->print(", \"airquality\": ");
  mqttClient->print(sensorReadings->gasResistance);
  mqttClient->print(", \"light\": ");
  mqttClient->print(sensorReadings->lightIntensity);
  mqttClient->print(", \"sequence\": ");
  mqttClient->print(sensorReadings->sequence);
  mqttClient->print(", \"timestamp\": ");
  mqttClient->print(sensorReadings->timestamp);
  mqttClient->print("}");

  bool status = mqttClient->endMessage();
  return status > 0;
}

bool sendSensorReadingsToMqttBroker(const SensorReadings* sensorReadings, MqttClient* mqttClient) {
  return sendSingleMeasurementMqttMessage(sensorReadings->temperature, "°C", sensorReadings->sequence, sensorReadings->timestamp, MQTT_TOPIC_TEMPERATURE, mqttClient)
         && sendSingleMeasurementMqttMessage(sensorReadings->pressure, "hPa", sensorReadings->sequence, sensorReadings->timestamp, MQTT_TOPIC_PRESSURE, mqttClient)
         && sendSingleMeasurementMqttMessage(sensorReadings->humidity, "%", sensorReadings->sequence, sensorReadings->timestamp, MQTT_TOPIC_HUMIDITY, mqttClient)
         && sendSingleMeasurementMqttMessage(sensorReadings->gasResistance, "KOhm", sensorReadings->sequence, sensorReadings->timestamp, MQTT_TOPIC_AIR_QUALITY, mqttClient)
         && sendSingleMeasurementMqttMessage(sensorReadings->lightIntensity, "%", sensorReadings->sequence, sensorReadings->timestamp, MQTT_TOPIC_LIGHT_INTENSITY, mqttClient)
         && sendCompleteSensorReadingsMqttMessage(sensorReadings, MQTT_TOPIC_ALL_MEASUREMENTS, mqttClient);
}

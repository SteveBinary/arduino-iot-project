#pragma once

#include <Wire.h>
#include <WiFiNINA.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define TEMPERATURE_OFFSET -1.0

#define SENSOR_READINGS_OVERSAMPLING_COUNT 10      // The average of 10 samples will be the resulting sensor reading.
#define SENSOR_READINGS_OVERSAMPLING_DELAY_MS 600  // The amount of milliseconds to wait between each sensor reading.
// Because of the delay when reading the sensors, 10 samples and waiting 600ms each time will end up approx. as a 10 second delay in total.


/**
* Contains the following fields:
*
* - float temperature (°C)
*
* - float pressure (hPa)
*
* - float humidity (%)
*
* - float gasResistance (KOhm)
*
* - float lightIntensity (%)
*
* - unsigned long timestamp (unix time in seconds)
*
* - unsigned long sequence
*/
struct SensorReadings {
  float temperature;
  float pressure;
  float humidity;
  float gasResistance;
  float lightIntensity;
  unsigned long timestamp;
  unsigned long sequence;
};

/**
* Initializing BME sensor.
* Returns true on success and false if initializing fails.
*/
bool initSensor(Adafruit_BME680* bmeSensor) {
  Serial.print("Initializing sensors ... ");

  if (!bmeSensor->begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    return false;
  }

  // Set up oversampling and filter initialization
  bmeSensor->setTemperatureOversampling(BME680_OS_8X);
  bmeSensor->setHumidityOversampling(BME680_OS_2X);
  bmeSensor->setPressureOversampling(BME680_OS_4X);
  bmeSensor->setIIRFilterSize(BME680_FILTER_SIZE_3);
  bmeSensor->setGasHeater(320, 150);  // 320*C for 150 ms

  Serial.println("done.");

  return true;
}

SensorReadings sensorReadingsOversamplingBuffer[SENSOR_READINGS_OVERSAMPLING_COUNT];

/*
* Reads oversampled sensor values into out_sensorReadings.
* Returns true on success and false if the reading fails.
*/
bool readSensorsOversampled(SensorReadings* out_sensorReadings, unsigned long sequence, Adafruit_BME680* bmeSensor, int lightSensorPin) {
  // perform all readings and store them into sensorReadingsOversamplingBuffer

  unsigned long beginTime = WiFi.getTime();

#ifdef DEBUG
  Serial.print("Begin oversampled reading of sensors at ");
  Serial.print(beginTime);
  Serial.print(" ... ");
#endif

  for (int i = 0; i < SENSOR_READINGS_OVERSAMPLING_COUNT; ++i) {
    if (!bmeSensor->performReading()) {
#ifdef DEBUG
      Serial.println("Error while reading sensors!");
#endif
      return initSensor(bmeSensor);
    }

    int lightSensorAnalogReading = analogRead(lightSensorPin);

    sensorReadingsOversamplingBuffer[i] = {
      temperature: bmeSensor->temperature + TEMPERATURE_OFFSET,   // °C
      pressure: bmeSensor->pressure / 100.0,                      // hPa
      humidity: bmeSensor->humidity,                              // percentage
      gasResistance: bmeSensor->gas_resistance / 1000.0,          // KOhm
      lightIntensity: lightSensorAnalogReading * (100 / 1023.0),  // percentage (normalize to be in [0, 100] to represent percentage)
      timestamp: -1,
      sequence: -1,
    };

    delay(SENSOR_READINGS_OVERSAMPLING_DELAY_MS);
  }

  unsigned long endTime = WiFi.getTime();

#ifdef DEBUG
  Serial.print("Done. Ended at ");
  Serial.print(endTime);
  Serial.print(" and took ");
  Serial.print(endTime - beginTime);
  Serial.println(" seconds.");
#endif

  // compute the avarage sensor reading from sensorReadingsOversamplingBuffer

  for (int i = 1; i < SENSOR_READINGS_OVERSAMPLING_COUNT; ++i) {
    sensorReadingsOversamplingBuffer[0].temperature += sensorReadingsOversamplingBuffer[i].temperature;
    sensorReadingsOversamplingBuffer[0].pressure += sensorReadingsOversamplingBuffer[i].pressure;
    sensorReadingsOversamplingBuffer[0].humidity += sensorReadingsOversamplingBuffer[i].humidity;
    sensorReadingsOversamplingBuffer[0].gasResistance += sensorReadingsOversamplingBuffer[i].gasResistance;
    sensorReadingsOversamplingBuffer[0].lightIntensity += sensorReadingsOversamplingBuffer[i].lightIntensity;
  }

  out_sensorReadings->temperature = sensorReadingsOversamplingBuffer[0].temperature / SENSOR_READINGS_OVERSAMPLING_COUNT;
  out_sensorReadings->pressure = sensorReadingsOversamplingBuffer[0].pressure / SENSOR_READINGS_OVERSAMPLING_COUNT;
  out_sensorReadings->humidity = sensorReadingsOversamplingBuffer[0].humidity / SENSOR_READINGS_OVERSAMPLING_COUNT;
  out_sensorReadings->gasResistance = sensorReadingsOversamplingBuffer[0].gasResistance / SENSOR_READINGS_OVERSAMPLING_COUNT;
  out_sensorReadings->lightIntensity = sensorReadingsOversamplingBuffer[0].lightIntensity / SENSOR_READINGS_OVERSAMPLING_COUNT;
  out_sensorReadings->timestamp = beginTime + ((endTime - beginTime) / 2);
  out_sensorReadings->sequence = sequence;

  return true;
}

/**
* Print a SensorReadings struct to Serial out.
*/
void printSensorReadings(const SensorReadings* sensorReadings) {
  Serial.print("New sensor readings (sequence=");
  Serial.print(sensorReadings->sequence);
  Serial.print(", timestamp=");
  Serial.print(sensorReadings->timestamp);
  Serial.println("):");

  Serial.print("    temperature     = ");
  Serial.print(sensorReadings->temperature);
  Serial.println(" °C");

  Serial.print("    pressure        = ");
  Serial.print(sensorReadings->pressure);
  Serial.println(" hPa");

  Serial.print("    humidity        = ");
  Serial.print(sensorReadings->humidity);
  Serial.println(" %");

  Serial.print("    gas resistance  = ");
  Serial.print(sensorReadings->gasResistance);
  Serial.println(" KOhms");

  Serial.print("    light intensity = ");
  Serial.print(sensorReadings->lightIntensity);
  Serial.println(" %");
}

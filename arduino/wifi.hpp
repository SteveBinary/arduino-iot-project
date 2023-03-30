#pragma once

#include <WiFiNINA.h>
#include "secrets.h"


/**
* Check if the WiFi module is present.
* Returns true if it is present and false otherwise.
*/
bool checkWiFiModule() {
  Serial.print("Checking WiFi module ... ");

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    return false;
  }

  Serial.println("done.");

  return true;
}

/**
* Connect to WiFi.
* Return true on success and false if the connection could not be established.
*/
bool connectWifi() {
  WiFi.disconnect();
  WiFi.scanNetworks();

  Serial.print("Attempting to connect to the WiFi SSID: ");
  Serial.print(WIFI_SSID);
  Serial.print(" ... ");

  int status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  if (status == WL_CONNECTED) {
    Serial.println("done.");
    Serial.println("Network details:");
    Serial.print("    IP address = ");
    Serial.println(WiFi.localIP());
    Serial.print("    Gateway    = ");
    Serial.println(WiFi.gatewayIP());
    return true;
  } else {
    Serial.println("failed!");
    return false;
  }
}

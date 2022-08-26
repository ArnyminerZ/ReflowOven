#ifndef __WEB_H
#define __WEB_H

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "secrets.h"

const char* host = MDNS_HOST;
const char* ssid = STA_SSID;
const char* password = STA_PASS;

bool wifi_connected = false;

bool ota_update = false;
int ota_progress = 0;

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

void web_init() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {    
    wifi_connected = true;

    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(host);

    ArduinoOTA.onStart([]() {
      Serial.println("OTA Start");
      ota_update = true;
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA End");
      ota_update = false;
      ota_progress = 0;
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      ota_progress = (progress / (total / 100));
      Serial.printf("OTA Progress: %u%%\r", ota_progress);
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      ota_update = false;
      ota_progress = 0;
    });
    ArduinoOTA.begin();

    Serial.println("WiFi Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi Failed");
  }
}

void web_loop() {
  if (!wifi_connected)
    return;
  ArduinoOTA.handle();
}

#endif

#ifndef __ESP__COMPAT__
#define __ESP__COMPAT__

//#define DEBUG_MODE
#define EANBLE_LOGGING

#include <Arduino.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

#endif
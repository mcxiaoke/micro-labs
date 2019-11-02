#ifndef __ESP__COMPAT__
#define __ESP__COMPAT__

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#if defined(ESP8266)
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#endif

String listFiles();

#endif
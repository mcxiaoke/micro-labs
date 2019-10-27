#ifndef _NET_H
#define _NET_H

#ifndef NET_DEBUG_LOG
#define NET_DEBUG_LOG 1
#endif

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include "Arduino.h"
#include "internal.h"

String wifiHttpPost(const String&, const String&, WiFiClient& client);
String wifiHttpGet(const String&, WiFiClient& client);
String httpPost(const String&, const String&);
String httpGet(const String&);
String httpsPost(const String&, const String&);
String httpsGet(const String&);
String getWiFiDisconnectReason(int code);

#endif
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

String wifiHttpPost(String&, String&, WiFiClient& client);
String wifiHttpGet(String&, WiFiClient& client);
String httpPost(String&, String&);
String httpGet(String&);
String httpsPost(String&, String&);
String httpsGet(String&);
String getWiFiDisconnectReason(int code);

#endif
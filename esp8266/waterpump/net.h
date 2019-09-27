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

String httpsPost(String &, String &);
String httpsGet(String &);

String httpsPost(String &url, String &body) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (NET_DEBUG_LOG) {
    Serial.printf("[HTTP] POST, url: %s\n", url.c_str());
  }
  if (http.begin(client, url)) {  // HTTP
    // Serial.print("[HTTP] POST sending...\n");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    Serial.printf("[HTTP] POST, body: %s\n", body.c_str());
    int httpCode = http.POST(body);
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST, code: %d\n", httpCode);
      // file found at server
      String payload = http.getString();
      Serial.printf("[HTTP] POST, content: %s\n", payload.c_str());
      return payload;
    } else {
      Serial.printf("[HTTP] POST, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.print("[HTTP] POST failed.\n");
  }
  http.end();
  return "";
}

String httpsGet(String &url) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  Serial.printf("[HTTP] GET, url: %s\n", url.c_str());
  if (http.begin(client, url)) {  // HTTP
    // Serial.print("[HTTP] GET sending...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET, code: %d\n", httpCode);
      // file found at server
      String payload = http.getString();
      Serial.printf("[HTTP] GET, content: %s\n", payload.c_str());
      return payload;
    } else {
      Serial.printf("[HTTP] GET, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.print("[HTTP] GET failed.\n");
  }
  http.end();
  return "";
}

#endif
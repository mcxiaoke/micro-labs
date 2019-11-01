#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "ESPTime.h"
#include "config.h"

bool isMqttConnected();
bool isMqttReq(const char topic[]);
String getStatusTopic();
String getRespTopic();
String getReqTopic();
void mqttStatus(const String& text);
void mqttResp(const String& text);
void mqttConnect();
void mqttCheck();
void mqttBegin(MQTT_CALLBACK_SIGNATURE);
void mqttLoop();

#endif
#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <PubSubClient.h>
#include "config.h"
#include "utils.h"

bool isMqttConnected();
bool isMqttReq(const char topic[]);
String getStatusTopic();
String getLogTopic();
String getReqTopic();
void mqttStatus(const String& text);
void mqttLog(const String& text);
void mqttConnect();
void mqttCheck();
void mqttBegin(MQTT_CALLBACK_SIGNATURE);
void mqttLoop();

#endif
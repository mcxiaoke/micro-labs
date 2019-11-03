#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <PubSubClient.h>
#include "config.h"
#include "utils.h"

bool isMqttConnected();
bool isMqttCmd(const char topic[]);
String getStatusTopic();
String getLogTopic();
String getCmdTopic();
void mqttStatus(const String& text);
void mqttStatus2(const std::string& text);
void mqttLog(const String& text);
void mqttLog2(const std::string& text);
void mqttConnect();
void mqttCheck();
void mqttBegin(MQTT_CALLBACK_SIGNATURE);
void mqttLoop();

#endif
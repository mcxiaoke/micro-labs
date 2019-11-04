#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <PubSubClient.h>
#include "cmd.h"
#include "config.h"
#include "ext/string/string.hpp"
#include "utils.h"

using std::string;

using MQTT_CALLBACK_FUNC = std::function<void(char*, uint8_t*, unsigned int)>;

class MqttManager {
  static const unsigned int COMMAND_MAX_LENGTH = 32;

 public:
  MqttManager(const char* server,
              const int port,
              const char* username,
              const char* password);
  bool isConnected();
  String getUser();
  String getPass();
  String getClientId();
  void sendStatus(const String& text);
  void sendStatus2(const std::string& text);
  void sendLog(const String& text);
  void sendLog2(const std::string& text);
  void connect();
  void check();
  void begin(CMD_HANDLER_FUNC);
  void loop();

 private:
  const char* _server;
  const int _port;
  const char* _username;
  const char* _password;
  WiFiClient _client;
  PubSubClient* _mqtt;
  CMD_HANDLER_FUNC _handler;
  bool isCommand(const string& topic);
  void handleMessage(const char* topic,
                     const uint8_t* payload,
                     const unsigned int length);
  void sendOnline();
  string getStatusTopic();
  string getLogTopic();
  string getCmdTopic();
};

#endif
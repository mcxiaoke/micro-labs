#include "mqtt.h"

const char* mqttServer PROGMEM = MQTT_SERVER;
const char* mqttUser PROGMEM = MQTT_USER;
const char* mqttPass PROGMEM = MQTT_PASS;
const char* mqttClientId PROGMEM = MQTT_CLIENT_ID;
const int mqttPort = MQTT_PORT;

static WiFiClient wc;
static PubSubClient mqtt(mqttServer, mqttPort, wc);

bool isMqttConnected() {
  return mqtt.connected();
}

bool isMqttReq(const char topic[]) {
  return strcmp(getReqTopic().c_str(), topic) == 0;
}

String getMyTopic() {
  // pump/%DEVICE%
  String topic = "pump/";
  topic += WiFi.hostname();
  return topic;
}

String getStatusTopic() {
  // pump/%DEVICE%/status
  String topic = "pump/";
  topic += WiFi.hostname();
  topic += "/status";
  return topic;
}

String getRespTopic() {
  // pump/%DEVICE%/resp
  String topic = "pump/";
  topic += WiFi.hostname();
  topic += "/resp";
  return topic;
}

String getReqTopic() {
  // pump/%DEVICE%/req
  String topic = "pump/";
  topic += WiFi.hostname();
  topic += "/req";
  return topic;
}

void mqttOnline() {
  // online message retain
  bool ret = mqtt.publish(getStatusTopic().c_str(), "Pump is online", true);
  if (!ret) {
    LOGN("[MQTT] mqtt online sent failed.");
  }
}

void mqttStatus(const String& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = mqtt.publish(getStatusTopic().c_str(), text.c_str());
  if (ret) {
    // LOGN("[MQTT] mqtt message sent successful.");
  } else {
    LOGN("[MQTT] mqtt status sent failed.");
  }
}

void mqttResp(const String& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = mqtt.publish(getRespTopic().c_str(), text.c_str());
  if (!ret) {
    LOGN("[MQTT] mqtt resp sent failed.");
  }
}

void mqttConnect() {
  // Loop until we're reconnected
  int maxRetries = 10;
  while (!mqtt.connected() && maxRetries-- > 0) {
    LOGN("[MQTT] Connecting...");
    // Attempt to connect
    // offline will message retain
    if (mqtt.connect(mqttClientId, mqttUser, mqttPass, getStatusTopic().c_str(),
                     MQTTQOS2, true, "Pump is offline")) {
      LOGN("[MQTT] Connected");
      mqttOnline();
      mqtt.subscribe("test");
      mqtt.subscribe(getReqTopic().c_str());
      //   statusReport();
    } else {
      LOG("[MQTT] Connect failed, rc=");
      LOG(mqtt.state());
      LOGN("[MQTT] Connect try again in 5 seconds");
      // Wait 3 seconds before retrying
      delay(2000);
    }
  }
}

void mqttCheck() {
  // Loop until we're reconnected
  if (!mqtt.connected()) {
    LOGN("[MQTT] Retry connect...");
    // Attempt to connect
    if (mqtt.connect(mqttClientId, mqttUser, mqttPass)) {
      LOG("[MQTT] Reconnected to ");
      LOGN(mqttServer);
      mqttStatus("Pump is online again");
      mqtt.subscribe("test");
      mqtt.subscribe(getReqTopic().c_str());
      //   statusReport();
    } else {
      LOG("[MQTT] Reconnect failed, rc=");
      LOG(mqtt.state());
    }
  } else {
    // mqttPing();
  }
}

void mqttBegin(MQTT_CALLBACK_SIGNATURE) {
  mqtt.setCallback(callback);
  mqttConnect();
}

void mqttLoop() {
  mqtt.loop();
}
#include "mqtt.h"

const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT;

static WiFiClient wc;
static PubSubClient mqtt(mqttServer, mqttPort, wc);

String getMqttUser() {
  return MQTT_USER;
}

String getMqttPass() {
  return MQTT_PASS;
}

String getMqttClientId() {
  return getDevice();
}

bool isMqttConnected() {
  return mqtt.connected();
}

bool isMqttReq(const char topic[]) {
  return strcmp(getReqTopic().c_str(), topic) == 0;
}

String getMyTopic() {
  // pump/%DEVICE%
  String topic = "pump/";
  topic += getDevice();
  return topic;
}

String getStatusTopic() {
  // pump/%DEVICE%/status
  String topic = "pump/";
  topic += getDevice();
  topic += "/status";
  return topic;
}

String getLogTopic() {
  // pump/%DEVICE%/log
  String topic = "pump/";
  topic += getDevice();
  topic += "/logs";
  return topic;
}

String getReqTopic() {
  // pump/%DEVICE%/req
  String topic = "pump/";
  topic += getDevice();
  topic += "/req";
  return topic;
}

void mqttOnline() {
  // online message retain
  bool ret = mqtt.publish(getStatusTopic().c_str(), "Online", true);
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

void mqttLog(const String& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = mqtt.publish(getLogTopic().c_str(), text.c_str());
  if (!ret) {
    LOGN("[MQTT] mqtt resp sent failed.");
  }
}

void mqttConnect() {
  // Loop until we're reconnected
  int maxRetries = 10;
  while (!mqtt.connected() && maxRetries-- > 0) {
    LOGF("[MQTT] Connecting to mqtt://%s:%s@%s\n", getMqttUser().c_str(),
         getMqttPass().c_str(), mqttServer);
    // Attempt to connect
    // offline will message retain
    if (mqtt.connect(getMqttClientId().c_str(), getMqttUser().c_str(),
                     getMqttPass().c_str(), getStatusTopic().c_str(), MQTTQOS2,
                     true, "Offline")) {
      LOG("[MQTT] Connected to ");
      LOG(mqttServer);
      LOG(" as client:");
      LOGN(getMqttClientId());
      mqttOnline();
      mqtt.subscribe("test");
      mqtt.subscribe(getReqTopic().c_str());
      //   statusReport();
    } else {
      LOGN("[MQTT] Connect failed, rc=");
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
    if (mqtt.connect(getMqttClientId().c_str(), getMqttUser().c_str(),
                     getMqttPass().c_str())) {
      LOG("[MQTT] Reconnected to ");
      LOG(mqttServer);
      LOG(" as client:");
      LOGN(getMqttClientId());
      mqttOnline();
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
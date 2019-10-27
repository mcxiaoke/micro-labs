#define DEBUG_MODE 1
#define ENABLE_LOGGING 1

// not working:https://www.esp8266.com/viewtopic.php?f=160&t=15872&start=4
#define MQTT_KEEPALIVE 128
#define MQTT_MAX_PACKET_SIZE 1024

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
// #include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <Updater.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
// #include <user_interface.h>
#include "common/config.h"
#include "common/internal.h"
#include "common/net.h"
#include "libs/SimpleTimer.h"

const char* JSON_CONFIG_FILE PROGMEM = "/file/config.json";
const char* STATUS_FILE PROGMEM = "/file/status.json";
const char* NTP_SERVER PROGMEM = "ntp.ntsc.ac.cn";
const char* MQTT_TOPIC_TEST PROGMEM = "test";
const char* MQTT_TOPIC_STATUS PROGMEM = "pump/status";
const char* MQTT_TOPIC_CMD PROGMEM = "pump/cmd";
int ledPin = 2;    // d4 2
int pumpPin = D2;  // d2 io4

typedef struct {
  bool globalSwitchOn;
  unsigned long runInterval;       // ms
  unsigned long runDuration;       // ms
  unsigned long statusInterval;    // ms
  unsigned long mqttInterval;      // ms
  unsigned long watchDogInterval;  // ms
  unsigned long wifiInterval;      // ms
  unsigned long ntpInterval;       // ms
} Config;

typedef struct {
  unsigned long lastOnAt;
  unsigned long lastOffAt;
  unsigned long lastOnElapsed;
  unsigned int totalCounter;
  unsigned long totalElapsed;
  unsigned long startedMs;
} Status;

Config config = {true, 0};
Status status = {0};

bool wifiInitialized = false;
unsigned long progressPrintMs;

int runTimerId, statusTimerId, mqttTimerId, wifiTimerId, watchdogTimerId;

String getStatusJson(bool);
String getStatusText();
String listFiles();
void showESP();
void fileLog(const String& text);
void loadConfig();
void saveStatus();
void showPumpTaskInfo();
void handlePump0(bool action);
void startPump();
void stopPump();
void pumpWatchDog();
void pumpReport();
void statusReport();
void mqttPing();
void mqttSend(const String& text);
void mqttConnect();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void setupMQTT();

SimpleTimer timer;
// ESP8266WiFiMulti wifiMgr;
WiFiEventHandler wh1, wh2, wh3;
WiFiClient wc;
PubSubClient mqtt(mqttServer, mqttPort, mqttCallback, wc);
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, NTP_SERVER, 0, 8 * 60 * 60 * 1000L);

void showESP() {
  LOGF("[System] Free Stack: %d, Free Heap: %d\n", ESP.getFreeContStack(),
       ESP.getFreeHeap());
}

void fileLog(const String& text) {
  LOGN(text);
  fileLog(text, true);
}

void saveStatus() {
  File file = SPIFFS.open(STATUS_FILE, "w");
  if (file) {
    file.println(getStatusJson(true));
    file.close();
    // LOGN(F("[Log] Write status file ok."));
  } else {
    LOGN(F("[Log] Write status file failed."));
  }
}

void loadConfig() {
  File file = SPIFFS.open(STATUS_FILE, "r");
  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    LOGN(F("[Config] Failed to load json config."));
  } else {
    config.globalSwitchOn = doc["switch"] | true;
    status.lastOnAt = doc["lastAt2"] | 0;
    status.totalElapsed = doc["totalElapsed"] | 0;
  }
  LOGN(F("[Config] loadConfig: "));
  serializeJsonPretty(doc, Serial);
  LOGN();
  file.close();
}

void setOTAUpdated() {
  // flag at 1023;
  EEPROM.write(1023, 1);
  EEPROM.commit();
}

byte isOTAUpdated() {
  // read and reset to 0
  byte b = EEPROM.read(1023);
  EEPROM.write(1023, 0);
  EEPROM.commit();
  return b;
}

time_t ntpSync() {
  ntp.update();
  LOG(F("[NTP] Server time is "));
  LOGN(dateTimeString(ntp.getEpochTime()));
  if (ntp.getEpochTime() < TIME_START) {
    // invalid time, failed, retry after 5s
    // timer.setTimeout(5 * 1000L, ntpSync);
    LOGN(F("[NTP] Server time invalid."));
    return 0;
  }
  LOG(F("[NTP] Synced, Curent Time:"));
  LOG(dateTimeString(ntp.getEpochTime()));
  LOG(F(", Up Time: "));
  LOGN(humanTimeMs(millis()));
  return ntp.getEpochTime();
}

void ntpUpdate() {
  ntpSync();
  if (ntp.getEpochTime() > TIME_START) {
    setTime(ntp.getEpochTime());
  }
}

void onWiFiConnected(const WiFiEventStationModeConnected& evt) {
  digitalWrite(ledPin, HIGH);
}

void onWiFiDisconnected(const WiFiEventStationModeDisconnected& evt) {
  digitalWrite(ledPin, LOW);
  LOG(F("[WiFi] Connection lost from "));
  LOG(evt.ssid);
  LOG(F(", Reason:"));
  LOGN(getWiFiDisconnectReason(evt.reason));
}

void onWiFiGotIP(const WiFiEventStationModeGotIP& evt) {
  digitalWrite(ledPin, HIGH);
  String msg = "[WiFi] Connected to ";
  msg += WiFi.SSID();
  msg += " IP: ";
  msg += WiFi.localIP().toString();
  if (wifiInitialized) {
    fileLog(msg);
    mqttSend(msg);
  } else {
    LOGN(msg);
  }
}

void showPumpTaskInfo() {
  LOGF("[Pump] Now: %s\n", nowString().c_str());
  LOGF("[Pump] Last: %s, elapsed %s\n",
       dateTimeString(now() - timer.getElapsed(runTimerId) / 1000).c_str(),
       humanTimeMs(timer.getElapsed(runTimerId)).c_str());
  LOGF("[Pump] Next: %s, remain %s\n",
       dateTimeString(now() + timer.getRemain(runTimerId) / 1000).c_str(),
       humanTimeMs(timer.getRemain(runTimerId)).c_str());
}

void handlePump0(bool turnOn) {
  if (!config.globalSwitchOn) {
    LOG(F("[Pump] global switch is off, ignore on."));
    return;
  }
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (turnOn == isOn) {
    LOGN(F("[Pump] status not changed, ignore."));
    return;
  }
  LOG(F("[Pump] Pump last status is "));
  LOG(isOn ? "Running" : "Idle");
  if (isOn) {
    // will stop
    unsigned long elapsed = 0;
    if (status.startedMs > 0) {
      // in seconds
      elapsed = (millis() - status.startedMs) / 1000.0;
    }
    LOG(", Elapsed: ");
    LOG(elapsed);
    LOG("s");
    status.startedMs = 0;
    status.lastOffAt = now();
    status.lastOnElapsed = elapsed;
  } else {
    // will start
    status.startedMs = millis();
    status.lastOnAt = now();
    status.lastOnElapsed = 0;
  }
  LOGN();
  digitalWrite(pumpPin, turnOn ? HIGH : LOW);
  saveStatus();
  timer.setTimeout(200L, pumpReport);
}

void pumpWatchDog() {
  //   bool isOn = digitalRead(pumpPin) == 1;
  //   LOGF("[Watchdog] pump lastOn: %s, now: %s status: %s mqtt: %d\n",
  //        timeString(status.lastOnAt).c_str(), timeString().c_str(),
  //        isOn ? "On" : "Off", mqtt.state());
  //   showESP();
  if (status.startedMs > 0 &&
      millis() - status.startedMs > config.runDuration) {
    if (digitalRead(pumpPin) == HIGH) {
      handlePump0(false);
      fileLog(F("[Pump] Stopped by watchdog."));
    }
  }
}

void startPump() {
  //   LOGN("[Pump] startPump at " + nowString());
  handlePump0(true);
  timer.setTimeout(config.runDuration, stopPump);
}
void stopPump() {
  //   LOGN("[Pump] stopPump at " + nowString());
  handlePump0(false);
}

String getStatusJson(bool pretty) {
  StaticJsonDocument<512> doc;
  doc["device"] = WiFi.hostname();
  doc["ts"] = now();
  doc["up"] = millis() / 1000;
  doc["name"] = WiFi.hostname();
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  //   doc["mac"] = WiFi.macAddress();
  doc["on"] = digitalRead(pumpPin) == HIGH;
  doc["lastElapsed"] = status.lastOnElapsed;
  doc["totalElapsed"] = status.totalElapsed;
  // timer or manual last run at
  doc["lastAt2"] = status.lastOnAt;
  // timer task last run at
  doc["lastAt"] = now() - timer.getElapsed(runTimerId) / 1000;
  doc["nextAt"] = now() + timer.getRemain(runTimerId) / 1000;
  doc["interval"] = config.runInterval;
  doc["duration"] = config.runDuration;
  doc["wifiPeriod"] = config.wifiInterval;
  doc["statusPeriod"] = config.statusInterval;
  doc["switch"] = config.globalSwitchOn;
  doc["stack"] = ESP.getFreeContStack();
  doc["heap"] = ESP.getFreeHeap();

#ifdef DEBUG_MODE
  doc["debug"] = true;
#endif
  String json;
  if (pretty) {
    serializeJsonPretty(doc, json);
  } else {
    serializeJson(doc, json);
  }

  return json;
}

String getStatusText() {
  String desp = "Device=";
  desp += WiFi.hostname();
  desp += "\nSwitch=";
  desp += config.globalSwitchOn ? "On" : "Off";
  desp += ("\nIP=");
  desp += (WiFi.localIP().toString());
  desp += ("\nSSID=");
  desp += (WiFi.SSID());
  desp += ("\nWiFi=");
  desp += (WiFi.status());
  desp += "\nDateTime=";
  desp += nowString();
  desp += ("\nUpTime=");
  desp += humanTimeMs(millis());
  desp += ("\nlastOffAt=");
  desp += (timeString(status.lastOffAt));
  desp += ("\nlastRunAt2=");
  desp += (timeString(status.lastOnAt));
  desp += ("\nlastRunAt=");
  desp += (timeString(now() - timer.getElapsed(runTimerId) / 1000));
  desp += ("\nnextRunAt=");
  desp += timeString(now() + timer.getRemain(runTimerId) / 1000);
  desp += ("\nlastOnElapsed=");
  desp += humanTime(status.lastOnElapsed);
  desp += ("\ntotalElapsed=");
  desp += humanTime(status.totalElapsed);
  desp += ("\nrunInterval=");
  desp += humanTimeMs(config.runInterval);
  desp += ("\nrunDuration=");
  desp += humanTimeMs(config.runDuration);
  desp += "\nStack=";
  desp += ESP.getFreeContStack();
  desp += "\nHeap=";
  desp += ESP.getFreeHeap();
#ifdef DEBUG_MODE
  desp += "\nDebug=true";
#endif
  return desp;
}

void setupNTP() {
  //   configTime(8 * 3600L, 0, "ntp.ntsc.ac.cn", "cn.ntp.org.cn",
  //              "cn.pool.ntp.org");
  ntp.begin();
  int retry = 100;
  LOGN(F("[NTP] Sync, get time from server."));
  ntpUpdate();
  while (timeStatus() != timeSet && --retry > 0) {
    delay(1000);
    LOGN(F("[NTP] Sync, get time failed, retry"));
    ntpUpdate();
  }
  if (timeStatus() != timeSet) {
    // ntp udpate failed, reboot
    ESP.restart();
  }
  LOGN(nowString());
}

void setupWiFi() {
  digitalWrite(ledPin, LOW);
  //   wifiMgr.addAP(ap1_ssid, ap1_pass);
  //   wifiMgr.addAP(ap2_ssid, ap2_pass);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ap1_ssid, ap1_pass);
  wh1 = WiFi.onStationModeConnected(&onWiFiConnected);
  wh2 = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
  wh3 = WiFi.onStationModeGotIP(&onWiFiGotIP);
  LOGN(F("[WiFi] Connecting......"));
  int retry = 100;
  while (WiFi.status() != WL_CONNECTED && --retry > 0) {
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    // wifi failed, reboot
    ESP.restart();
  }
  wifiInitialized = true;
}

void checkWiFi() {
  LOG("[WiFi] Status: ");
  LOG(WiFi.status());
  LOG(", SSID: ");
  LOG(WiFi.SSID());
  LOG(", IP: ");
  LOG(WiFi.localIP());
  //   LOG(", RSSI: ");
  //   LOG(WiFi.RSSI());
  LOG(", Time: ");
  LOGN(timeString());
  saveStatus();
  if (!WiFi.isConnected()) {
    LOGN(F("[WiFi] connection lost, reconnecting..."));
    WiFi.reconnect();
  }
}

char* statusReportText() {
  char* buf = (char*)malloc(1024);
  int pos = 0;
  pos += snprintf(buf, 1024, "%s%s", "Device=", WiFi.hostname().c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nSwitch=", config.globalSwitchOn ? "On" : "Off");
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nIP=", WiFi.localIP().toString().c_str());
  pos +=
      snprintf(buf + pos, 1024 - pos, "%s%s", "\nSSID=", WiFi.SSID().c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%d", "\nWiFi=", WiFi.status());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nDateTime=", nowString().c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nUpTime=", humanTimeMs(millis()).c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nlastAt2=", timeString(status.lastOnAt).c_str());
  pos +=
      snprintf(buf + pos, 1024 - pos, "%s%s", "\nlastAt=",
               timeString(now() - timer.getElapsed(runTimerId) / 1000).c_str());
  pos +=
      snprintf(buf + pos, 1024 - pos, "%s%s", "\nnextAt=",
               timeString(now() + timer.getRemain(runTimerId) / 1000).c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\nlastOnElapsed=", humanTime(status.lastOnElapsed).c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%s",
                  "\ntotalElapsed=", humanTime(status.totalElapsed).c_str());
  pos += snprintf(buf + pos, 1024 - pos, "%s%d",
                  "\nStack=", ESP.getFreeContStack());
  pos += snprintf(buf + pos, 1024 - pos, "%s%d", "\nHeap=", ESP.getFreeHeap());

  size_t len = strlen(buf);
  LOGN(buf);
  LOGN(len);
  char* text = (char*)malloc(len + 1);
  memset(text, 0, len + 1);
  memcpy(text, buf, len);
  free(buf);
  return text;
}

void statusReport() {
  mqttSend(getStatusText());
}

void mqttPing() {
  mqtt.publish(MQTT_TOPIC_TEST, "ok");
}

void mqttSend(const String& text) {
  LOGF("[MQTT] send message: %s\n", text.c_str());
  bool ret = mqtt.publish(MQTT_TOPIC_STATUS, text.c_str());
  if (ret) {
    // LOGN("[MQTT] mqtt message sent successful.");
  } else {
    LOGN("[MQTT] mqtt message sent failed.");
  }
}

void mqttConnect() {
  // Loop until we're reconnected
  int maxRetries = 10;
  while (!mqtt.connected() && maxRetries-- > 0) {
    LOGN("[MQTT] MQTT Connecting...");
    // Attempt to connect
    if (mqtt.connect(mqttClientId, mqttUser, mqttPass)) {
      fileLog("[MQTT] Connected to " + String(mqttServer));
      mqttSend("Pump is online at " + nowString());
      mqtt.subscribe(MQTT_TOPIC_TEST);
      mqtt.subscribe(MQTT_TOPIC_CMD);
      //   mqtt.subscribe(MQTT_TOPIC_STATUS);
      statusReport();
    } else {
      LOG("[MQTT] failed, rc=");
      LOG(mqtt.state());
      LOGN(" try again in 5 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

void mqttReconnect() {
  // Loop until we're reconnected
  if (!mqtt.connected()) {
    LOGN("[MQTT] retry connect...");
    // Attempt to connect
    if (mqtt.connect(mqttClientId, mqttUser, mqttPass)) {
      LOG("[MQTT] Reconnected to");
      LOGN(mqttServer);
      fileLog("[MQTT] Reconnected to " + String(mqttServer));
      mqttSend("Pump is online again at " + nowString());
      mqtt.subscribe(MQTT_TOPIC_TEST);
      mqtt.subscribe(MQTT_TOPIC_CMD);
      //   mqtt.subscribe(MQTT_TOPIC_STATUS);
      statusReport();
    } else {
      LOG("[MQTT] reconnect failed, rc=");
      LOG(mqtt.state());
    }
  } else {
    mqttPing();
  }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  char* message = (char*)malloc(length + 1);
  memset(message, 0, length + 1);
  memcpy(message, payload, length);
  LOGF("[MQTT] Message arrived [%s] (%s) <%d>\n", message, topic,
       strlen(message));
  if (strEqual(topic, MQTT_TOPIC_CMD)) {
    // command received, handle it
    if (strEqual(message, "start")) {
      LOGN("[MQTT] cmd:start");
      startPump();
    } else if (strEqual(message, "stop")) {
      LOGN("[MQTT] cmd:stop");
      stopPump();
    } else if (strEqual(message, "on")) {
      LOGN("[MQTT] cmd:on");
      config.globalSwitchOn = true;
      mqttSend("Global switch on");
    } else if (strEqual(message, "off")) {
      LOGN("[MQTT] cmd:off");
      config.globalSwitchOn = false;
      mqttSend("Global switch off");
    } else if (strEqual(message, "wifi")) {
      LOGN("[MQTT] cmd:wifi");
      String wf = "IP=";
      wf += WiFi.localIP().toString();
      wf += "\nSSID=";
      wf += WiFi.SSID();
      wf += "\nConnected=";
      wf += WiFi.isConnected() ? "True" : "False";
      mqttSend(wf);
    } else if (strEqual(message, "status")) {
      LOGN("[MQTT] cmd:status");
      statusReport();
    } else if (strEqual(message, "files")) {
      LOGN("[MQTT] cmd:files");
      mqttSend(listFiles());
    } else if (strEqual(message, "logs")) {
      LOGN("[MQTT] cmd:logs");
      String logText = "---LOG BEGIN---\n";
      logText += readLog(logFileName());
      logText += "\n----LOG END----\n";
      mqttSend(logText);
    } else {
      LOGN("[MQTT] cmd:unknown");
      mqttSend("Error: unknown command");
    }
  }
  free(message);
  showESP();
}

void setupMQTT() {
  mqttConnect();
}

String listFiles() {
  String output = "";
  LOGN(F("[System] SPIFFS Files:"));
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    LOGF("[File] %s (%d bytes)\n", dir.fileName().c_str(), dir.fileSize());
    output += dir.fileName();
    // output += " (";
    // output += dir.fileSize();
    // output += ")";
    output += "\n";
  };
  return output;
}

// 1 minutes per day, 2 times, 30 seconds per one run
// total water elapsed 12m = 720s, 720/10=72, 72/4=18
void setupData() {
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  bool debugMode = false;
#ifdef DEBUG_MODE
  debugMode = true;
  config.runInterval = 3 * 60 * 1000L;               // 3min
  config.runDuration = 15 * 1000L;                   // 15s
  config.statusInterval = 5 * 60 * 1000L;            // 5min
  config.watchDogInterval = config.runDuration / 2;  // 7s;
  config.ntpInterval = 8 * 60 * 1000L;               // 8min
  config.wifiInterval = 2 * 60 * 1000L;              // 2min
  config.mqttInterval = 2 * 60 * 1000L;              // 2min
#else
  debugMode = false;
  config.runInterval = 8 * 3600 * 1000L;             // 8 * 3 = 24 hours
  config.runDuration = 20 * 1000L;                   // 60/3=20 seconds
  config.statusInterval = 3600 * 1000L;              // 1hours
  config.watchDogInterval = config.runDuration / 2;  // 10s
  config.ntpInterval = 2 * 3600 * 1000L;             // 2 hours
  config.wifiInterval = 30 * 60 * 1000L;             // 30min
  config.mqttInterval = 5 * 60 * 1000L;              // 5min
#endif
  LOGF("[System] setupData, debug=%s\n", debugMode ? "True" : "False");
}

void setupTimers() {
  LOGN(F("[System] Setup timers."));
  setSyncInterval(config.ntpInterval / 1000);  // in seconds
  setSyncProvider(ntpSync);
  wifiTimerId = timer.setInterval(config.wifiInterval, checkWiFi);
  runTimerId = timer.setInterval(config.runInterval, startPump);
  watchdogTimerId = timer.setInterval(config.watchDogInterval, pumpWatchDog);
  mqttTimerId = timer.setInterval(config.mqttInterval, mqttReconnect);
  statusTimerId = timer.setInterval(config.statusInterval, statusReport);
}

void setup() {
  // Free Stack: 3904, Free Heap: 42000
  Serial.begin(115200);
  LOGN();
  delay(1000);
  LOGN(F("============ SETUP BEGIN ============"));
  showESP();
  LOGN(F("[System] Board is booting on..."));
  EEPROM.begin(1024);
  if (!SPIFFS.begin()) {
    LOGN(F("[System] Failed to mount file system"));
  } else {
    LOGN(F("[System] SPIFFS file system mounted."));
  }
  if (isOTAUpdated()) {
    LOGN(F("[System] Firmware OTA Updated!"));
  }

  loadConfig();
  setupData();
  setupWiFi();
  setupNTP();
  setupTimers();
  setupMQTT();
  saveStatus();
  fileLog("[System] Board boot on " + nowString());
  showESP();
  LOGN(F("============= SETUP END ============="));
}

void loop() {
  mqtt.loop();
  timer.run();
}

void pumpReport() {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    status.totalCounter++;
  } else {
    status.totalElapsed += status.lastOnElapsed;
    LOGN("[Server] Pump is scheduled at " +
         dateTimeString(now() + config.runInterval / 1000));
  }
  String desp = "[Pump]";
  desp += isOn ? " Started" : " Stopped";
  desp += " at ";
  desp += timeString(isOn ? status.lastOnAt : status.lastOffAt);
  desp += ", Total:";
  desp += humanTime(status.totalElapsed);
  if (!isOn) {
    desp += ", Last:";
    desp += humanTime(status.lastOnElapsed);
  }
  fileLog(desp);
  saveStatus();
  showESP();
  //   statusReport();
  mqttSend(desp);
}

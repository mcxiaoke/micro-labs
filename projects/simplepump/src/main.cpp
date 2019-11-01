//#define DEBUG_MODE
#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "ESPTime.h"
#include "SimpleTimer.h"
#include "mqtt.h"

#undef LED_BUILTIN
#define LED_BUILTIN 2

#ifdef DEBUG_MODE
const unsigned long RUN_INTERVAL = 60 * 1000UL;
const unsigned long RUN_DURATION = 18 * 1000UL;
#else
const unsigned long RUN_INTERVAL = 6 * 3600 * 1000UL;
const unsigned long RUN_DURATION = 15 * 1000UL;
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const int led = LED_BUILTIN;
const int pump = D5;

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

int runTimerId, lcdTimerId, mqttTimerId, statusTimerId;

static const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";

void doEnable();
void doDisable();
void startPump();
void stopPump();
void checkPump();
void checkWiFi();
String getStatus();
void statusReport();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
void setupDisplay();
void updateDisplay(const String& line1, const String& line2);
void updateDisplay(const char line1[], const char line2[]);
void displayStatus();

LiquidCrystal_I2C lcd(0x27, 16, 2);
SimpleTimer timer;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

bool strEqual(const char* str1, const char* str2) {
  return strcmp(str1, str2) == 0;
}

void showESP() {
  Serial.printf("Free Stack: %d, Free Heap: %d\n", ESP.getFreeContStack(),
                ESP.getFreeHeap());
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  yield();
  char* message = (char*)malloc(length + 1);
  memset(message, 0, length + 1);
  memcpy(message, payload, length);
  LOGF("[MQTT] Message arrived [%s] (%s) <%d>\n", message, topic,
       strlen(message));
  if (isMqttReq(topic)) {
    // command received, handle it
    if (strEqual(message, "start")) {
      LOGN("[MQTT] cmd:start");
      startPump();
    } else if (strEqual(message, "stop")) {
      LOGN("[MQTT] cmd:stop");
      stopPump();
    } else if (strEqual(message, "on")) {
      LOGN("[MQTT] cmd:on");
      doEnable();
    } else if (strEqual(message, "off")) {
      LOGN("[MQTT] cmd:off");
      doDisable();
    } else if (strEqual(message, "wifi")) {
      LOGN("[MQTT] cmd:wifi");
      String wf = "IP=";
      wf += WiFi.localIP().toString();
      wf += "\nSSID=";
      wf += WiFi.SSID();
      wf += "\nConnected=";
      wf += WiFi.status();
      mqttStatus(wf);
    } else if (strEqual(message, "status")) {
      LOGN("[MQTT] cmd:status");
      statusReport();
    } else if (strEqual(message, "system")) {
      String msg = "Free Stack: ";
      msg += ESP.getFreeContStack();
      msg += ", Free Heap: ";
      msg += ESP.getFreeHeap();
      mqttStatus(msg);
    } else if (strEqual(message, "logs")) {
      LOGN("[MQTT] cmd:logs");
      String logText = "Go to http://";
      logText += WiFi.localIP().toString();
      logText += "/file/log.txt";
      mqttStatus(logText);
    } else {
      LOGN("[MQTT] cmd:unknown");
      mqttResp("Error: unknown command");
    }
  }
  free(message);
  showESP();
}

void setupDisplay() {
  lcd.init();
  lcd.backlight();
  updateDisplay("WiFi Connecting", ".........");
}

void updateDisplay(const String& line1, const String& line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void updateDisplay(const char line1[], const char line2[]) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void clearDisplay() {
  lcd.clear();
}

void displayStatus() {
  char line1[17] = {0};
  char line2[17] = {0};
  bool isOn = digitalRead(pump) == HIGH;
  const time_t nextRun = getTimestamp() + timer.getRemain(runTimerId) / 1000;
  snprintf(line1, 17, "Next: %s", formatTime(nextRun).c_str());
  snprintf(line2, 17, "Pump:%s WiFi:%s", isOn ? "ON" : "OF",
           WiFi.isConnected() ? "ON" : "OF");
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

size_t debugLog(const String& text) {
  String msg = "[";
  msg += dateTimeString();
  msg += "] ";
  msg += text;
  Serial.println(msg);
  mqttResp(msg);
  File f = SPIFFS.open("/file/log.txt", "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(msg);
  c += f.print('\n');
  f.close();
  return c;
}

void checkHttpUpdate() {
  WiFiClient client;
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  t_httpUpdate_return ret = ESPhttpUpdate.update(
      client, F("http://192.168.1.109:8000/esp/firmware.bin"));
  // Or:
  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80,
  // "file.bin");

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
                    ESPhttpUpdate.getLastError(),
                    ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("HTTP_UPDATE_OK"));
      break;
  }
}

void doEnable() {
  timer.enable(runTimerId);
  debugLog("Timer enabled");
}
void doDisable() {
  timer.disable(runTimerId);
  debugLog("Timer disabled");
}

void startPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  debugLog("Pump started");
  mqttStatus("Pump started");
  timer.setTimeout(RUN_DURATION, stopPump);
}

void stopPump() {
  bool isOff = digitalRead(pump) == LOW;
  if (isOff) {
    return;
  }
  lastStop = millis();
  if (lastStart > 0) {
    lastSeconds = (lastStop - lastStart) / 1000;
    totalSeconds += lastSeconds;
  }
  digitalWrite(pump, LOW);
  digitalWrite(led, HIGH);
  String msg = "Pump stopped,last:";
  msg += lastSeconds;
  msg += "s,total:";
  msg += totalSeconds;
  msg += "s";
  debugLog(msg);
  mqttStatus(msg);
}

void checkPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn && lastStart > 0 && (millis() - lastStart) / 1000 >= RUN_DURATION) {
    debugLog(F("Pump stopped by watchdog"));
    stopPump();
  }
}

void checkWiFi() {
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog("WiFi reconnect");
  }
}

String getStatus() {
  auto ts = millis();
  String data = "";
  data += "Device: ";
  data += WiFi.hostname();
  data += "\nTimer: ";
  data += timer.isEnabled(runTimerId) ? "Enabled" : "Disabled";
  data += "\nStatus: ";
  data += (digitalRead(pump) == HIGH) ? "Running" : "Idle";
  data += "\nMQTT: ";
  data += isMqttConnected() ? "Connected" : "Disconnected";
  data += "\nFree Stack: ";
  data += ESP.getFreeContStack();
  data += "\nFree Heap: ";
  data += ESP.getFreeHeap();
  data += "\nLast Elapsed: ";
  data += lastSeconds;
  data += "s\nTotal Elapsed: ";
  data += totalSeconds;
  data += "s\nSystem Boot: ";
  data += formatDateTime(getTimestamp() - ts / 1000);
  if (lastStart > 0) {
    data += "\nLast Start: ";
    data += formatDateTime(getTimestamp() - (ts - lastStart) / 1000);
  }
  if (lastStop > 0) {
    data += "\nLast Stop: ";
    data += formatDateTime(getTimestamp() - (ts - lastStop) / 1000);
  }
  data += "\nNext Start: ";
  data += formatDateTime(getTimestamp() + timer.getRemain(runTimerId) / 1000);
  return data;
}

void statusReport() {
  mqttStatus(getStatus());
}

void handleStart() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    startPump();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleStop() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    stopPump();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleClear() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    SPIFFS.remove("/file/log.txt");
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void reboot() {
  ESP.reset();
}

void handleReboot() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    timer.setTimeout(1000, reboot);
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleDisable() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    doDisable();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleEnable() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    doEnable();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleRoot() {
  server.send(200, "text/plain", getStatus().c_str());
  showESP();
}

void setupWiFi() {
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  Serial.print("WiFi Connecting");
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 30 * 1000L) {
    delay(1000);
    Serial.print(".");
  }
  if (WiFi.isConnected()) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(led, HIGH);
  } else {
    Serial.println("WiFi connect failed");
  }
}

void setupDate() {
  initESPTime();
}

void setupServer() {
  if (MDNS.begin("esp8266")) {
    debugLog(F("MDNS responder started"));
  }
  server.on("/", handleRoot);
  server.on("/api/status", handleRoot);
  server.on("/reboot", handleReboot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/clear", handleClear);
  server.on("/disable", handleDisable);
  server.on("/enable", handleEnable);
  server.on("/on", handleEnable);
  server.on("/off", handleDisable);
  server.serveStatic("/file/", SPIFFS, "/file/");
  server.serveStatic("/www/", SPIFFS, "/www/");
  server.serveStatic("/log", SPIFFS, "/file/log.txt");
  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  debugLog(F("HTTP server started"));
}

void setupTimers() {
  runTimerId = timer.setInterval(RUN_INTERVAL, startPump);
  timer.setInterval(RUN_DURATION / 2 + 2000, checkPump);
  timer.setInterval((MQTT_KEEPALIVE * 2 - 3) * 1000L, mqttCheck);
  timer.setInterval(5 * 60 * 1000L, checkWiFi);
  timer.setInterval(60 * 60 * 1000L, statusReport);
  timer.setInterval(5 * 1000L, displayStatus);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  setupDisplay();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println(F("Failed to mount file system"));
  } else {
    Serial.println(F("SPIFFS file system mounted."));
  }
  setupWiFi();
  setupDate();
  debugLog(F("System initialized"));
  mqttBegin(mqttCallback);
  setupServer();
  setupTimers();
  clearDisplay();
  displayStatus();
}

void loop(void) {
  mqttLoop();
  server.handleClient();
  MDNS.update();
  timer.run();
}

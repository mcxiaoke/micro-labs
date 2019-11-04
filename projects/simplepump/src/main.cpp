#include <Arduino.h>
#include <Wire.h>
#include "ext/string/string.hpp"
#include "libs/ESPUpdateServer.h"
#include "libs/FileServer.h"
#include "libs/SimpleTimer.h"
#include "libs/cmd.h"
#include "libs/compat.h"
#include "libs/config.h"
#include "libs/mqtt.h"
#include "libs/net.h"
#include "libs/utils.h"

// #undef PSTR
// #define PSTR

using std::string;

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
const int pump = 14;  // nodemcu D5

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

int runTimerId, mqttTimerId, statusTimerId;
const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";

SimpleTimer timer;
#if defined(ESP8266)
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif
ESPUpdateServer otaUpdate(true);
CommandManager cmdMgr;
MqttManager mqttMgr(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

void startPump();
void stopPump();
void checkPump();
void checkWiFi();
String getCommands();
String getStatus();
void statusReport();
void onMqttMessage(const string&, string& message);
void handleCommand(std::vector<string> args);

size_t debugLog(const String text) {
  String msg = "[";
  msg += dateTimeString();
  msg += "] ";
  msg += text;
  LOGN(msg);
  mqttMgr.sendLog(text);
  File f = SPIFFS.open(logFileName(), "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(msg);
  c += f.print('\n');
  f.close();
  return c;
}

////////// Command Handlers Begin //////////

void cmdEnable(const vector<string> args = vector<string>()) {
  timer.enable(runTimerId);
  debugLog(F("Timer enabled"));
}
void cmdDisable(const vector<string> args = vector<string>()) {
  timer.disable(runTimerId);
  debugLog(F("Timer disabled"));
}

void cmdStart(const vector<string> args = vector<string>()) {
  LOGN("cmdStart");
  startPump();
}

void cmdStop(const vector<string> args = vector<string>()) {
  LOGN("cmdStop");
  stopPump();
}

void cmdClear(const vector<string> args = vector<string>()) {
  SPIFFS.remove(logFileName());
  debugLog(F("Logs cleared"));
}

void cmdFiles(const vector<string> args = vector<string>()) {
  LOGN("cmdFiles");
  mqttMgr.sendStatus(listFiles());
}

void cmdLogs(const vector<string> args = vector<string>()) {
  LOGN("cmdLogs");
  String logText = F("Go to http://");
  logText += WiFi.localIP().toString();
  logText += logFileName();
  mqttMgr.sendStatus(logText);
}

void cmdStatus(const vector<string> args = vector<string>()) {
  LOGN("cmdStatus");
  statusReport();
}

void cmdWiFi(const vector<string> args = vector<string>()) {
  LOGN("cmdWiFi");
  String wf = "Mac=";
  wf += WiFi.macAddress();
  wf += "\nIP=";
  wf += WiFi.localIP().toString();
  wf += "\nSSID=";
  wf += WiFi.SSID();
  wf += "\nStatus=";
  wf += WiFi.status();
  mqttMgr.sendStatus(wf);
}

uint8_t parsePin(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 2 ||
      !extstring::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

uint8_t parseValue(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 1 ||
      !extstring::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

void cmdIOSet(const vector<string> args) {
  LOGF("cmdIOSet");
  if (args.size() < 3) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
  if (pin == (uint8_t)0xff) {
    mqttMgr.sendLog(F("Invalid Pin"));
    return;
  }
  uint8_t value = parseValue(args[2]);
  if (value > (uint8_t)1) {
    mqttMgr.sendLog(F("Invalid Value"));
    return;
  }
}

void cmdIOSetHigh(const vector<string> args) {
  LOGF("cmdIOSetHigh");
  if (args.size() < 2) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
  if (pin > (uint8_t)0x80) {
    mqttMgr.sendLog("Invalid Pin");
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  String msg = F("cmdIOSetHigh for Pin ");
  msg += pin;
  debugLog(msg);
}

void cmdIOSetLow(const vector<string> args) {
  LOGF("cmdIOSetLow");
  if (args.size() < 2) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
  if (pin > (uint8_t)0x80) {
    mqttMgr.sendLog("Invalid Pin");
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  String msg = F("cmdIOSetLow for Pin ");
  msg += pin;
  debugLog(msg);
}

void cmdHelp(const vector<string> args = vector<string>()) {
  LOGN("cmdHelp");
  mqttMgr.sendStatus2(cmdMgr.getHelpDoc());
}

void cmdNotFound(const vector<string> args = vector<string>()) {
  LOGN("cmdNotFound");
  mqttMgr.sendLog("What?");
}

/////////// Command Handlers End ///////////

void startPump() {
  LOGN("startPump");
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  timer.setTimeout(RUN_DURATION, stopPump);
  debugLog(F("Pump started"));
  mqttMgr.sendStatus(F("Pump started"));
}

void stopPump() {
  LOGN("stopPump");
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
  String msg = F("Pump stopped,last:");
  msg += lastSeconds;
  msg += "s,total:";
  msg += totalSeconds;
  msg += "s";
  debugLog(msg);
  mqttMgr.sendStatus(msg);
}

void checkPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn && lastStart > 0 && (millis() - lastStart) / 1000 >= RUN_DURATION) {
    debugLog(F("Pump stopped by watchdog"));
    cmdStop();
  }
}

void checkWiFi() {
  //   LOGN("checkWiFi");
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
}

void checkMqtt() {
  //   LOGN("checkMqtt");
  mqttMgr.check();
}

String getStatus() {
  auto ts = millis();
  String data = "";
  data += "Device: ";
  data += getDevice();
  data += "\nTimer: ";
  data += timer.isEnabled(runTimerId) ? "Enabled" : "Disabled";
  data += "\nStatus: ";
  data += (digitalRead(pump) == HIGH) ? "Running" : "Idle";
  data += "\nMQTT: ";
  data += mqttMgr.isConnected() ? "Connected" : "Disconnected";
  data += "\nWiFi: ";
  data += WiFi.localIP().toString();
#if defined(ESP8266)
  data += "\nFree Stack: ";
  data += ESP.getFreeContStack();
#endif
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
  LOGN("statusReport");
  mqttMgr.sendStatus(getStatus());
}

void handleFiles() {
  LOGN("handleFiles");
  server.send(200, MIME_TEXT_PLAIN, listFiles());
}

void handleLogs() {
  LOGN("handleLogs");
  File file = SPIFFS.open(logFileName(), "r");
  server.streamFile(file, MIME_TEXT_PLAIN);
}

void handleStart() {
  LOGN("handleStart");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStart();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop() {
  LOGN("handleStop");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStop();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleClear() {
  LOGN("handleClear");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdClear();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void reboot() {
  ESP.restart();
}

void handleReboot() {
  LOGN("handleReboot");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    timer.setTimeout(1000, reboot);
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleDisable() {
  LOGN("handleDisable");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdDisable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleEnable() {
  LOGN("handleEnable");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdEnable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleRoot() {
  LOGN("handleRoot");
  server.send(200, MIME_TEXT_PLAIN, getStatus().c_str());
  showESP();
  //   String data = "text=";
  //   data += urlencode("Pump_Status_Report_");
  //   data += urlencode(getDevice());
  //   data += "&desp=";
  //   data += urlencode(getStatus());
  //   httpsPost(WX_REPORT_URL, data);
}

void handleIOSet() {
  for (auto i = 0; i < server.args(); i++) {
    LOGF("handleIOSet %s=%s\n", server.argName(i).c_str(),
         server.arg(i).c_str());
  }
  //   if (server.hasArg("pin") && server.hasArg("value")) {}
}

void setupWiFi() {
  LOGN("setupWiFi");
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  //   WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
#if defined(ESP8266)
//   WiFi.hostname(getDevice().c_str());
#elif defined(ESP32)
//   WiFi.setHostname(getDevice().c_str());
#endif
  WiFi.begin(ssid, password);
  Serial.print("WiFi Connecting");
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 30 * 1000L) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
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
  LOGN("setupDate");
  setTimestamp();
}

void setupUpdate() {
  LOGN("setupUpdate");
  otaUpdate.setup(&server);
}

void handleNotFound() {
  LOGN("handleNotFound");
  if (!FileServer::handle(&server)) {
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += server.uri();
    data += "\n";
    server.send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer() {
  LOGN("setupServer");
  if (MDNS.begin(getDevice().c_str())) {
    LOGN(F("[Server] MDNS responder started"));
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
  server.on("/ioset", handleIOSet);
  server.on("/files", handleFiles);
  server.on("/logs", handleLogs);
  server.onNotFound(handleNotFound);
  setupUpdate();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Server] HTTP server started"));
}

void setupTimers() {
  LOGN("setupTimers");
  runTimerId = timer.setInterval(RUN_INTERVAL, startPump);
  timer.setInterval(RUN_DURATION / 2 + 2000, checkPump);
  timer.setInterval((MQTT_KEEPALIVE * 2 - 3) * 1000L, checkMqtt);
  timer.setInterval(5 * 60 * 1000L, checkWiFi);
  timer.setInterval(60 * 60 * 1000L, statusReport);
}

void setupCommands() {
  LOGN("setupCommands");
  cmdMgr.setDefaultHandler(cmdNotFound);
  cmdMgr.addCommand("on", "set timer on", cmdEnable);
  cmdMgr.addCommand("enable", "set timer on", cmdEnable);
  cmdMgr.addCommand("off", "set timer off", cmdDisable);
  cmdMgr.addCommand("disable", "set timer off", cmdDisable);
  cmdMgr.addCommand("start", "start pump", cmdStart);
  cmdMgr.addCommand("stop", "stop pump", cmdStop);
  cmdMgr.addCommand("status", "get pump status", cmdStatus);
  cmdMgr.addCommand("wifi", "get wifi status", cmdWiFi);
  cmdMgr.addCommand("logs", "show logs", cmdLogs);
  cmdMgr.addCommand("files", "show files", cmdFiles);
  cmdMgr.addCommand("ioset", "gpio set [pin] [value]", cmdIOSet);
  cmdMgr.addCommand("ioset1", "gpio set to 1 for [pin]", cmdIOSetHigh);
  cmdMgr.addCommand("ioset0", "gpio set to 0 for  [pin]", cmdIOSetLow);
  cmdMgr.addCommand("list", "show available commands", cmdHelp);
  cmdMgr.addCommand("help", "show help message", cmdHelp);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  showESP();
  fsCheck();
  setupCommands();
  setupWiFi();
  setupDate();
  setupServer();
  setupTimers();
  showESP();
  mqttMgr.begin(handleCommand);
  debugLog(F("System Boot"));
}

void loop(void) {
  mqttMgr.loop();
  server.handleClient();
#if defined(ESP8266)
  MDNS.update();
#endif
  timer.run();
}

void handleCommand(std::vector<string> args) {
  auto processFunc = [args] {
    showESP();
    CommandParam param{args[0].substr(1), args};
    LOG("[CMD] handleCommand ");
    LOGN(param.toString().c_str());
    cmdMgr.handle(param);
    yield();
    showESP();
  };
  timer.setTimeout(5, processFunc);
}

#include <Arduino.h>
#include <Wire.h>
#include "libs/FileServer.h"
#include "libs/SimpleTimer.h"
#include "libs/cmd.h"
#include "libs/compat.h"
#include "libs/config.h"
#include "libs/cpptools.h"
#include "libs/mqtt.h"
#include "libs/net.h"
#include "libs/utils.h"

#undef PSTR
#define PSTR

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
#if defined(ESP32)
const char* UPDATE_INDEX PROGMEM =
    "<form method='POST' action='/update' enctype='multipart/form-data'><input "
    "type='file' name='update'><input type='submit' value='Update'></form>";
#endif
const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";

SimpleTimer timer;
#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
WebServer server(80);
#endif
CommandManager cmdMgr;

void startPump();
void stopPump();
void checkPump();
void checkWiFi();
String getCommands();
String getStatus();
void statusReport();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);

size_t debugLog(const String text) {
  String msg = "[";
  msg += dateTimeString();
  msg += "] ";
  msg += text;
  LOGN(msg);
  mqttLog(text);
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
  mqttStatus(listFiles());
}

void cmdLogs(const vector<string> args = vector<string>()) {
  LOGN("cmdLogs");
  String logText = F("Go to http://");
  logText += WiFi.localIP().toString();
  logText += logFileName();
  mqttStatus(logText);
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
  mqttStatus(wf);
}

uint8_t parsePin(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 2 ||
      !is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

uint8_t parseValue(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 1 ||
      !is_digits(extra)) {
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
    mqttLog(F("Invalid Pin"));
    return;
  }
  uint8_t value = parseValue(args[2]);
  if (value > (uint8_t)1) {
    mqttLog(F("Invalid Value"));
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
    mqttLog("Invalid Pin");
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
    mqttLog("Invalid Pin");
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
  mqttStatus2(cmdMgr.getHelpDoc());
}

void cmdNotFound(const vector<string> args = vector<string>()) {
  LOGN("cmdNotFound");
  mqttLog("What?");
}

/////////// Command Handlers End ///////////

void startPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  timer.setTimeout(RUN_DURATION, stopPump);
  debugLog(F("Pump started"));
  mqttStatus(F("Pump started"));
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
  String msg = F("Pump stopped,last:");
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
    cmdStop();
  }
}

void checkWiFi() {
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
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
  data += isMqttConnected() ? "Connected" : "Disconnected";
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
  mqttStatus(getStatus());
}

void handleFiles() {
  server.send(200, MIME_TEXT_PLAIN, listFiles());
}

void handleLogs() {
  File file = SPIFFS.open(logFileName(), "r");
  server.streamFile(file, MIME_TEXT_PLAIN);
}

void handleStart() {
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStart();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop() {
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStop();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleClear() {
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
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    timer.setTimeout(1000, reboot);
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleDisable() {
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdDisable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleEnable() {
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdEnable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleRoot() {
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
  setTimestamp();
}

void setupUpdate() {
#if defined(ESP8266)
  httpUpdater.setup(&server);
#elif defined(ESP32)
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", UPDATE_INDEX);
  });
  server.on(
      "/update", HTTP_POST,
      []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },
      []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);
          int command =
              (upload.filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
          Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
          Serial.printf("[OTA] Update Type: %s\n",
                        command == U_FLASH ? "Firmware" : "Spiffs");
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, command, led)) {
            // start with max available size
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          // true to set the size to the current progress
          if (Update.end(true)) {
            Serial.printf("[OTA] Update Success: %u\nRebooting...\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        } else {
          Serial.printf(
              "[OTA] Update Failed Unexpectedly (likely broken connection): "
              "status=%d\n",
              upload.status);
        }
      });
#endif
}

void handleNotFound() {
  if (!handleFileRead(&server)) {
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += server.uri();
    data += "\n";
    server.send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer() {
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
  runTimerId = timer.setInterval(RUN_INTERVAL, startPump);
  timer.setInterval(RUN_DURATION / 2 + 2000, checkPump);
  timer.setInterval((MQTT_KEEPALIVE * 2 - 3) * 1000L, mqttCheck);
  timer.setInterval(5 * 60 * 1000L, checkWiFi);
  timer.setInterval(60 * 60 * 1000L, statusReport);
}

void setupCommands() {
  cmdMgr.setDefaultHandler(cmdNotFound);
  cmdMgr.addCommand("on", PSTR("set timer on"), cmdEnable);
  cmdMgr.addCommand("enable", PSTR("set timer on"), cmdEnable);
  cmdMgr.addCommand("off", PSTR("set timer off"), cmdDisable);
  cmdMgr.addCommand("disable", PSTR("set timer off"), cmdDisable);
  cmdMgr.addCommand("start", PSTR("start pump"), cmdStart);
  cmdMgr.addCommand("stop", PSTR("stop pump"), cmdStop);
  cmdMgr.addCommand("status", PSTR("get pump status"), cmdStatus);
  cmdMgr.addCommand("wifi", PSTR("get wifi status"), cmdWiFi);
  cmdMgr.addCommand("logs", PSTR("show logs"), cmdLogs);
  cmdMgr.addCommand("files", PSTR("show files"), cmdFiles);
  cmdMgr.addCommand("ioset", PSTR("gpio set [pin] [value]"), cmdIOSet);
  cmdMgr.addCommand("ioset1", PSTR("gpio set to 1 for [pin]"), cmdIOSetHigh);
  cmdMgr.addCommand("ioset0", PSTR("gpio set to 0 for  [pin]"), cmdIOSetLow);
  cmdMgr.addCommand("list", PSTR("show available commands"), cmdHelp);
  cmdMgr.addCommand("help", PSTR("show help message"), cmdHelp);
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
  mqttBegin(mqttCallback);
  setupServer();
  setupTimers();
  debugLog(F("System initialized"));
  showESP();
}

void loop(void) {
  mqttLoop();
  server.handleClient();
#if defined(ESP8266)
  MDNS.update();
#endif
  timer.run();
}

bool checkCommand(string message) {
  static const char* CMD_PREFIX = "/#@!$%";
  return message.length() > 2 && strchr(CMD_PREFIX, message.at(0)) != nullptr;
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  string message(payload, payload + length);
  str_replace(message, "\n", " ");
  LOGF("[MQTT][%s] Message: [%s] - '%s' (%d)\n", timeString().c_str(), topic,
       message.substr(0, 64).c_str(), length);
  if (!isMqttCmd(topic)) {
    mqttLog("What?");
    return;
  }
  trim(message);
  if (!checkCommand(message)) {
    LOGN(F("[MQTT] Invalid Command"));
    mqttLog("What?");
    return;
  }

  auto processFunc = [message] {
    vector<string> args = split2(message, " ", false, true);
    CommandParam param{args[0].substr(1), args};
    // LOGN("[CMD] Processing command");
    // LOGN(param.toString().c_str());
    cmdMgr.handle(param);
  };
  timer.setTimeout(5, processFunc);
}

#include <Arduino.h>
#include <Wire.h>
#include "libs/FileServer.h"
#include "libs/SimpleTimer.h"
#include "libs/compat.h"
#include "libs/config.h"
#include "libs/cpptools.h"
#include "libs/mqtt.h"
#include "libs/net.h"
#include "libs/utils.h"

using std::string;

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
// const int pump = D5;
const int pump = 12;

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

int runTimerId, mqttTimerId, statusTimerId;
#if defined(ESP32)
const char* UPDATE_INDEX =
    "<form method='POST' action='/update' enctype='multipart/form-data'><input "
    "type='file' name='update'><input type='submit' value='Update'></form>";
#endif
const char REBOOT_RESPONSE[] =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";

// using CMD_HANDLER_FUNC = std::function<void(char*, char*)>;
using CMD_HANDLER_FUNC = std::function<void()>;
std::map<std::string, CMD_HANDLER_FUNC> cmdHandlers;
SimpleTimer timer;
#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
WebServer server(80);
#endif

void cmdEnable();
void cmdDisable();
void cmdStart();
void cmdStop();
void checkPump();
void checkWiFi();
String getStatus();
void mqttCallback(char* topic, uint8_t* payload, unsigned int length);

size_t debugLog(const String& text) {
  String msg = "[";
  msg += dateTimeString();
  msg += "] ";
  msg += text;
  LOGN(msg);
  mqttLog(msg);
  File f = SPIFFS.open("/file/log.txt", "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(msg);
  c += f.print('\n');
  f.close();
  return c;
}

////////// Command Handlers Begin //////////

void cmdEnable() {
  timer.enable(runTimerId);
  debugLog("Timer enabled");
}
void cmdDisable() {
  timer.disable(runTimerId);
  debugLog("Timer disabled");
}

void cmdStart() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  debugLog("Pump started");
  mqttStatus("Pump started");
  timer.setTimeout(RUN_DURATION, cmdStop);
}

void cmdStop() {
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

void cmdClear() {
  SPIFFS.remove("/file/log.txt");
  debugLog("Logs cleared");
}

void cmdFiles() {
  LOGN("cmdFiles");
  mqttStatus(listFiles());
}

void cmdLogs() {
  LOGN("cmdLogs");
  String logText = "Go to http://";
  logText += WiFi.localIP().toString();
  logText += "/file/log.txt";
  mqttStatus(logText);
}

void cmdStatus() {
  LOGN("cmdStatus");
  mqttStatus(getStatus());
}

void cmdWiFi() {
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

void cmdNotFound() {
  LOGN("cmdNotFound");
  mqttLog("What?");
}

/////////// Command Handlers End ///////////

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
    debugLog("WiFi Reconnect");
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

void handleFiles() {
  server.send(200, "text/plain", listFiles());
}

void handleStart() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    cmdStart();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleStop() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    cmdStop();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleClear() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    cmdClear();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void reboot() {
  ESP.restart();
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
    cmdDisable();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleEnable() {
  if (server.hasArg("do")) {
    server.send(200, "text/plain", "ok");
    cmdEnable();
  } else {
    server.send(200, "text/plain", "ignore");
  }
}

void handleRoot() {
  server.send(200, "text/plain", getStatus().c_str());
  showESP();
  //   String data = "text=";
  //   data += urlencode("Pump_Status_Report_");
  //   data += urlencode(getDevice());
  //   data += "&desp=";
  //   data += urlencode(getStatus());
  //   httpsPost(WX_REPORT_URL, data);
}

void setupWiFi() {
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  //   WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
#if defined(ESP8266)
  WiFi.hostname(getDevice().c_str());
#elif defined(ESP32)
  WiFi.setHostname(getDevice().c_str());
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
    String data = "ERROR: 404 NOT FOUND\nURI: ";
    data += server.uri();
    data += "\n";
    server.send(404, "text/plain", data);
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
  server.on("/files", handleFiles);
  //   server.serveStatic("/file/", SPIFFS, "/file/");
  //   server.serveStatic("/www/", SPIFFS, "/www/");
  server.serveStatic("/log", SPIFFS, "/file/log.txt");
  server.onNotFound(handleNotFound);
  setupUpdate();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Server] HTTP server started"));
}

void setupTimers() {
  runTimerId = timer.setInterval(RUN_INTERVAL, cmdStart);
  timer.setInterval(RUN_DURATION / 2 + 2000, checkPump);
  timer.setInterval((MQTT_KEEPALIVE * 2 - 3) * 1000L, mqttCheck);
  timer.setInterval(5 * 60 * 1000L, checkWiFi);
  timer.setInterval(60 * 60 * 1000L, cmdStatus);
}

void registerHandler(std::string cmd, CMD_HANDLER_FUNC func) {
  cmdHandlers.insert(std::pair<std::string, CMD_HANDLER_FUNC>(cmd, func));
}

void setupCommands() {
  registerHandler("on", cmdEnable);
  registerHandler("enable", cmdEnable);
  registerHandler("off", cmdDisable);
  registerHandler("disable", cmdDisable);
  registerHandler("start", cmdStart);
  registerHandler("stop", cmdStop);
  registerHandler("status", cmdStatus);
  registerHandler("wifi", cmdWiFi);
  registerHandler("logs", cmdLogs);
  registerHandler("files", cmdFiles);
  registerHandler("404", cmdNotFound);
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
  yield();
  string message(payload, payload + length);
  LOGF("[MQTT] Received [%s]:'%s' (%d)\n", topic, message.substr(0, 64).c_str(),
       length);
  if (!isMqttCmd(topic)) {
    return;
  }
  ltrim(message);
  if (!checkCommand(message)) {
    LOGN("[MQTT] Invalid Command");
    mqttLog("What?");
    return;
  }

  vector<string> tokens = split2(message, " ", true);
  string cmd;
  string extra;
  if (tokens.size() > 1) {
    cmd = tokens[0];
    extra = tokens[1];
  } else {
    cmd = tokens[0];
    extra = "";
  }
  LOGF("[CMD] cmd='%s', extra='%s'\n", cmd.c_str(), extra.c_str());
  CMD_HANDLER_FUNC func404(cmdNotFound);
  GetWithDef(cmdHandlers, cmd.substr(1), func404)();
}

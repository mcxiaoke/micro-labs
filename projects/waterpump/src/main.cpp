//#define DEBUG_MODE 1

#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <FS.h>
#define BLYNK_PRINT Serial
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <TimeLib.h>
// #include <WebSerial.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>

#include "common/config.h"
#include "common/data.h"
#include "common/internal.h"
#include "common/net.h"

// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const char* PUMP_LOG_FILE = "/file/pump.log";

int ledPin = 2;    // d4 2
int pumpPin = D2;  // d2 io4

unsigned long runInterval;
unsigned long runDuration;
unsigned long reportInterval;
unsigned long checkWifiInterval;

bool wifiInitialized = false;
bool ntpInitialized = false;
unsigned long pumpStartedMs = 0;
unsigned long pumpLastModifiedMs = 0;

unsigned long pumpLastOnAt = 0;
unsigned long pumpLastOffAt = 0;
unsigned long pumpLastOnElapsed = 0;
unsigned int pumpTotalCounter = 0;
unsigned long pumpTotalElapsed = 0;

String cmdOutput;

BlynkTimer timer;
WidgetTerminal terminal(V9);
// libraries/ESP8266WiFi/src/ESP8266WiFiType.h
WiFiEventHandler wh1, wh2, wh3;

void statusReport();
void pumpReport();
String listFiles();
void webSerialRecv(uint8_t* data, size_t len);
void handleWebSerialCmd(const String& cmd);
void fileLog(const String& text, bool);
void fileLog(const String& text);
String rootProcessor(const String& var);
String logsProcessor(const String& var);
String cmdProcessor(const String& var);
void handleNotFound(AsyncWebServerRequest* req);
void handleRoot(AsyncWebServerRequest* req);
void handleLogs(AsyncWebServerRequest* req);
void handlePump(AsyncWebServerRequest* req);
void handlePump0(bool action);
void startPump();
void stopPump();
void handleClear(AsyncWebServerRequest* req);
void handleReset(AsyncWebServerRequest* req);
void handleCmdGet(AsyncWebServerRequest* req);
void handleCmdPost(AsyncWebServerRequest* req);

const char* ntpServer = "ntp.ntsc.ac.cn";
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, ntpServer, 3600L * 8, 60 * 60 * 1000L);

// ESP Async Web Server Begin
AsyncWebServer server(80);
// ESP Async Web Server End

void fileLog(const String& text) {
  fileLog(text, true);
}

void fileLog(const String& text, bool appendDate) {
  String message = "";
  if (appendDate) {
    message += "[";
    message += nowString();
    message += "] ";
  }
  message += text;
  writeLog(PUMP_LOG_FILE, message);
  terminal.println(message);
}

void ntpSetup() {
  ntp.begin();
}

time_t ntpSync() {
  ntp.update();
  yield();
  if (ntp.getEpochTime() < TIME_START) {
    // invalid time
    return 0;
  }
  Serial.print("[NTP] Time:");
  Serial.print(ntp.getFormattedTime());
  Serial.print(", Epoch:");
  Serial.print(ntp.getEpochTime());
  Serial.print(", Millis: ");
  Serial.println(millis());
  ntpInitialized = true;
  return ntp.getEpochTime();
}

void ntpUpdate() {
  ntpSync();
  if (timeStatus() != timeSet) {
    setTime(ntp.getEpochTime());
  }
}

void onWiFiConnected(const WiFiEventStationModeConnected& evt) {
  digitalWrite(ledPin, LOW);
}

void onWiFiDisconnected(const WiFiEventStationModeDisconnected& evt) {
  digitalWrite(ledPin, HIGH);
  Serial.print("[WiFi] Connection lost from ");
  Serial.print(evt.ssid);
  Serial.print(", Reason:");
  Serial.println(getWiFiDisconnectReason(evt.reason));
}

void onWiFiGotIP(const WiFiEventStationModeGotIP& evt) {
  digitalWrite(ledPin, LOW);
  String msg = "[WiFi] Connected to ";
  msg += WiFi.SSID();
  msg += " IP: ";
  msg += WiFi.localIP().toString();
  Serial.println(msg);
  if (wifiInitialized) {
    fileLog(msg);
  }
}

String rootProcessor(const String& var) {
  //   Serial.println("rootProcessor var=" + var);
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (var == "DATE_TIME") {
    return timeString();
  } else if (var == "PUMP_LABEL") {
    return isOn ? "Running" : "Idle";
  } else if (var == "PUMP_ACTION") {
    return isOn ? "off" : "on";
  } else if (var == "PUMP_SUBMIT") {
    return isOn ? "Stop Pump" : "Start Pump";
  } else if (var == "PUMP_RUN_AT") {
    return pumpLastOnAt > 0 ? dateTimeString(pumpLastOnAt) : "N/A";
  } else if (var == "PUMP_RUN_ELAPSED") {
    return pumpLastOnElapsed > 0 ? String(pumpLastOnElapsed) + "s" : "N/A";
  } else if (var == "PUMP_INTERVAL") {
    return elapsedFormatMs(runInterval);
  } else if (var == "PUMP_DURATION") {
    return elapsedFormatMs(runDuration);
  }
  return String();
}

String cmdProcessor(const String& var) {
  Serial.println("cmdProcessor var=" + var);
  if (var == "CMD_OUTPUT") {
    String output = "";
    if (cmdOutput) {
      output += "<p>";
      cmdOutput.replace("\n", "</p><p>");
      output += cmdOutput;
      output += "</p>";
      //   Serial.println(output);
    }
    cmdOutput = "";
    return output;
  }
  return String();
}

String logsProcessor(const String& var) {
  Serial.println("logsProcessor var=" + var);
  if (var == "PUMP_OUTPUT") {
    String output = "<p>";
    String lines = readLog(PUMP_LOG_FILE);
    if (lines) {
      lines.replace("\n", "</p>\n<p>");
      output += lines;
    }
    output += "</p>";
    // Serial.println(output);
    return output;
  }
  return String();
}

void handleNotFound(AsyncWebServerRequest* request) {
  Serial.println("[Server] handleNotFound url: " + request->url());
  request->send(404, "text/plain", "Not found");
}

void handleLogs(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleLogs");
  req->send(SPIFFS, "/www/logs.html", String(), false, logsProcessor);
}

void handleRoot(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleRoot");
  //   req->send_P(200, "text/html", ROOT_PAGE_TPL, processor);
  req->send(SPIFFS, "/www/index.html", String(), false, rootProcessor);
}

void handlePump0(bool action) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (action == isOn) {
    return;
  }
  unsigned long nowTs = now();
  unsigned long elapsed = 0;
  if (pumpStartedMs > 0) {
    // in seconds
    elapsed = (millis() - pumpStartedMs + 1.0f) / 1000.0;
  }
  Serial.print("[Server] Pump is ");
  Serial.print(isOn ? "Running" : "Idle");
  // now is on, will off;
  if (isOn) {
    Serial.print(", Elapsed: ");
    Serial.print(elapsed);
    Serial.print("s");
    pumpStartedMs = 0;
    pumpLastOffAt = nowTs;
    pumpLastOnElapsed = elapsed;
  } else {
    pumpStartedMs = millis();
    pumpLastOnAt = nowTs;
    pumpLastOnElapsed = 0;
  }
  Serial.println();
  digitalWrite(pumpPin, action ? HIGH : LOW);
  pumpLastModifiedMs = millis();
  timer.setTimeout(100L, pumpReport);
}

void startPump() {
  Serial.println("[CMD] startPump at " + nowString());
  handlePump0(true);
  timer.setTimeout(runDuration, stopPump);
}
void stopPump() {
  Serial.println("[CMD] stopPump at " + nowString());
  handlePump0(false);
}

void handlePump(AsyncWebServerRequest* req) {
  if (pumpLastModifiedMs > 0 && millis() - pumpLastModifiedMs < 2000L) {
    Serial.println("[Server] Pump action too often, ignored.");
    req->redirect("/");
    return;
  }
  if (!req->hasArg("action")) {
    req->redirect("/");
    return;
  }
  String actionStr = req->arg("action");
  bool action = actionStr == "on";
  bool isOn = digitalRead(pumpPin) == HIGH;
  Serial.printf("[Server] Pump actionStr=%s, isOn=%d\n", actionStr.c_str(),
                isOn);
  handlePump0(action);
  req->redirect("/");
}

void handleClear(AsyncWebServerRequest* req) {
  bool removed = SPIFFS.remove(PUMP_LOG_FILE);
  Serial.print("[Server] Pump logs cleared, result: ");
  Serial.println(removed ? "success" : "fail");
  req->redirect("/logs");
}

void handleReset(AsyncWebServerRequest* req) {
  Serial.print("[Server] Pump is rebooting...");
  req->redirect("/");
  server.end();
  ESP.restart();
}

void handleCmdGet(AsyncWebServerRequest* req) {
  Serial.print("[Server] handleCmdGet: ");
  Serial.println(req->url());
  req->send(SPIFFS, "/www/cmd.html", String(), false, cmdProcessor);
}

void handleCmdPost(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleCmdPost");
  //   size_t n = req->params();
  //   for (size_t i = 0; i < n; i++) {
  //     AsyncWebParameter* p = req->getParam(i);
  //     Serial.printf("Param: %s = %s, isPost:%d\n", p->name().c_str(),
  //                   p->value().c_str(), p->isPost() ? 1 : 0);
  //   }
  if (!req->hasParam("action")) {
    req->redirect("/cmd");
    return;
  }
  AsyncWebParameter* action = req->getParam("action");
  if (action && action->value() == "fs_delete") {
    if (req->hasParam("fs_path", true)) {
      AsyncWebParameter* path = req->getParam("fs_path", true);
      String filePath = path->value();
      if (SPIFFS.remove(filePath)) {
        cmdOutput = "File '";
        cmdOutput += filePath;
        cmdOutput += "' is deleted.";
      }
    }
  } else if (action->value() == "fs_list") {
    cmdOutput = listFiles();
  }
  req->redirect("/cmd");
}

void setupServer() {
  if (MDNS.begin("esp8266")) {  // Start the mDNS responder for esp8266.local
    Serial.println("[Server] mDNS responder started");
  } else {
    Serial.println("[Server] Error setting up MDNS responder!");
  }

  // accessible at "<IP Address>/webserial" in browser
  //   WebSerial.begin(&server);
  //   WebSerial.msgCallback(webSerialRecv);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/pump", HTTP_POST, handlePump);
  server.on("/clear", HTTP_POST, handleClear);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/cmd", HTTP_GET, handleCmdGet);
  server.on("/cmd", HTTP_POST, handleCmdPost);
  server.serveStatic("/file/", SPIFFS, "/file/");
  server.serveStatic("/www/", SPIFFS, "/www/");
  server.serveStatic("/main.css", SPIFFS, "/www/main.css");
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[Server] HTTP server started");
}

void webSerialRecv(uint8_t* data, size_t len) {
  String s = "";
  for (size_t i = 0; i < len; i++) {
    s += char(data[i]);
  }
  //   WebSerial.println(s);
  Serial.print("WebSerial: ");
  Serial.println(s);
  handleWebSerialCmd(s);
}

void showHelpMsg() {
  /***
WebSerial.println("--------------------");
WebSerial.println("Available Commands:");
WebSerial.println("\t/help show this message.");
WebSerial.println("\t/logs show pump file logs");
WebSerial.println("\t/list show spiffs files");
WebSerial.println("\t/wifi show wifi info");
WebSerial.println("\t/connect do wifi reconnect");
WebSerial.println("\t/setio 11 0 set 1/0 for io port 11");
WebSerial.println("\t/reset reboot chip.");
WebSerial.println("--------------------");
***/
}

void handleWebSerialCmd(const String& cmd) {
  showHelpMsg();
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);  // no sleep
  WiFi.setAutoReconnect(true);
  wh1 = WiFi.onStationModeConnected(&onWiFiConnected);
  wh2 = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
  wh3 = WiFi.onStationModeGotIP(&onWiFiGotIP);
  Serial.println("[WiFi] Connecting......");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
  }
  wifiInitialized = true;
}

// for led pin
BLYNK_WRITE(V0)  // Button Widget is writing to pin V0
{
  int v = param.asInt();
  Serial.printf("[Blynk] value %d received at V0\n", v);
  digitalWrite(ledPin, !v);
}

// for pump pin
BLYNK_WRITE(V1)  // Button Widget is writing to pin V1
{
  int v = param.asInt();
  Serial.printf("[Blynk] value %d received at V1\n", v);
  handlePump0(v == HIGH);
}

// for get report
BLYNK_WRITE(V2)  // Button Widget is writing to pin V2
{
  int v = param.asInt();
  Serial.printf("[Blynk] value %d received at V2\n", v);
  if (v == HIGH) {
    statusReport();
  }
}

// for blynk terminal
BLYNK_WRITE(V9)  // Button Widget is writing to pin V9
{
  Serial.printf("[Blynk] value %s received at V9\n", param.asStr());
  if (String("/help") == param.asStr()) {
    terminal.println("Commands: /status, /start, /stop, /help");
  } else if (String("/status") == param.asStr()) {
    statusReport();
  } else if (String("/start") == param.asStr()) {
    startPump();
  } else if (String("/stop") == param.asStr()) {
    stopPump();
  }
}

BLYNK_CONNECTED() {
  Serial.println("[Blynk] Connected!");
  Blynk.syncAll();
}

void checkWiFi() {
  Serial.print("[WiFi] Status: ");
  Serial.print(WiFi.status());
  Serial.print(", SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print(", IP: ");
  Serial.print(WiFi.localIP());
  //   Serial.print(", RSSI: ");
  //   Serial.print(WiFi.RSSI());
  Serial.print(", Time: ");
  Serial.println(timeString());
  if (!WiFi.isConnected()) {
    Serial.println("[WiFi] connection lost, reconnecting...");
    WiFi.reconnect();
  } else {
    // statusReport();
    if (!ntpInitialized) {
      ntpUpdate();
    }
  }
}

String listFiles() {
  String output = "";
  Serial.println("[System] SPIFFS Files:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    Serial.printf("[File] %s (%d bytes)\n", dir.fileName().c_str(),
                  dir.fileSize());
    output += dir.fileName();
    output += " (";
    output += dir.fileSize();
    output += ")";
    output += "\r\n";
  };
  return output;
}

// 1 minutes per day, 2 times, 30 seconds per one run
// total water elapsed 12m = 720s, 720/10=72, 72/4=18
void setupData() {
  if (!SPIFFS.begin()) {
    Serial.println("[System] Failed to mount file system");
  }
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  bool debugMode = false;
#ifdef DEBUG_MODE
  debugMode = true;
  runInterval = 3 * 60 * 1000L;        // 3min
  runDuration = 12 * 1000L;            // 12s
  reportInterval = 5 * 60 * 1000L;     // 5min
  checkWifiInterval = 2 * 60 * 1000L;  // 2min
#else
  debugMode = false;
  runInterval = 8 * 3600 * 1000L;      // 8 * 3 = 24 hours
  runDuration = 20 * 1000L;            // 60/3=20 seconds
  reportInterval = 6 * 3600 * 1000L;   // 6hours
  checkWifiInterval = 5 * 60 * 1000L;  // 5min
#endif
  Serial.printf("[System] setupData, debugMode=%s\n",
                debugMode ? "True" : "False");
}

void setTimers() {
  setSyncInterval(24 * 60 * 60);  // in seconds = 1 days
  setSyncProvider(ntpSync);
  //   timer.setInterval(30 * 60 * 1000L, ntpUpdate);  // in milliseconds
  //   timer.setInterval(5 * 60 * 1000L, statusReport);
  timer.setInterval(checkWifiInterval, checkWiFi);  // in milliseconds
  timer.setInterval(runInterval, startPump);
  timer.setInterval(reportInterval, statusReport);
}

void setup() {
  // Debug console
  Serial.begin(115200);
  delay(500);
  Serial.println("[System] ESP8266 Board is booting...");
  //----------
  setupData();
  setupWiFi();
  setupServer();
  ntpSetup();
  ntpUpdate();
  setTimers();
  Blynk.begin(blynkAuth, ssid, pass);
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);
  fileLog("[System] ESP8266 Board initialized.");
  //   listFiles();
  statusReport();
}

void loop() {
  Blynk.run();
  timer.run();
}

void statusReport() {
  static int srCount = 0;
  String data = "text=";
  data += urlencode("Pump_Status_");
#ifdef DEBUG_MODE
  data += urlencode("DEBUG_");
#endif
  data += (++srCount);
  data += "&desp=";
  data += urlencode("Pump Server is running");
  data += urlencode(", DateTime: ");
  data += urlencode(nowString());
  data += urlencode(", UpTime: ");
  data += millis() / 1000;
  data += urlencode("s, Timestamp: ");
  data += now();
  data += urlencode(", SSID: ");
  data += urlencode(WiFi.SSID());
  data += urlencode(", IP:");
  data += urlencode(WiFi.localIP().toString());
  data += urlencode(", Config: [ ");
  data += urlencode("runInterval=");
  data += runInterval / 1000;
  data += urlencode("s, runDuration=");
  data += runDuration / 1000;
  data += urlencode("s, checkWifiInterval=");
  data += checkWifiInterval / 1000;
  data += urlencode("s ], Status: [ ");
  data += urlencode("lastRunAt=");
  data += urlencode(dateTimeString(pumpLastOnAt));
  data += urlencode(", lastRunDuration=");
  data += pumpLastOnElapsed;
  data += urlencode("s, totalDuration=");
  data += pumpTotalElapsed;
  data += "s ]";
  data += urlencode(" (No.");
  data += srCount;
  data += ")";
  Serial.printf("[Report] %s Status Report No.%2d is sent to server.\n",
                WiFi.hostname().c_str(), srCount);
#ifndef DEBUG_MODE
  httpsPost(reportUrl, data);
#endif
}

void pumpReport() {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    pumpTotalCounter++;
  } else {
    pumpTotalElapsed += pumpLastOnElapsed;
    Serial.println("[Server] Pump is scheduled at " +
                   dateTimeString(now() + runInterval));
  }
  bool debugMode = false;
#ifdef DEBUG_MODE
  debugMode = true;
#endif
  Serial.print("[Report] Pump is ");
  Serial.print(isOn ? "Started" : "Stopped");
  Serial.print(" at ");
  Serial.print(nowString());
  Serial.print(" debugMode: ");
  Serial.println(debugMode ? "True" : "False");

  String data = "text=";
  data += urlencode("Pump_");
  data += urlencode(isOn ? "Started" : "Stopped");
  data += urlencode(debugMode ? "_DEBUG_" : "_");
  data += pumpTotalCounter;
  data += "&desp=";

  String desp = "Pump";
  desp += isOn ? " Started" : " Stopped";
  desp += " at ";
  desp += timeString();
  desp += ", Total: ";
  desp += pumpTotalElapsed;
  desp += "s";
  if (!isOn) {
    desp += ", Elapsed: ";
    desp += pumpLastOnElapsed;
    desp += "s";
  }
  desp += " (";
  desp += pumpTotalCounter;
  desp += ")";
  fileLog(desp);
  data += urlencode(desp);
  Serial.printf("[Report] %s Action Report is sent to server.\n",
                WiFi.hostname().c_str());
#ifndef DEBUG_MODE
  httpsPost(reportUrl, data);
#endif
}

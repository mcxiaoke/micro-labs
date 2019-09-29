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
#include <WebSerial.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>

#include "common/config.h"
#include "common/data.h"
#include "common/internal.h"
#include "common/net.h"

// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

// 1 minutes per day, 2 times, 30 seconds per one run
// total water elapsed 12m = 720s, 720/10=72, 72/4=18
const unsigned long PUMP_RUN_INTERVAL = 8 * 3600 * 1000L;  // 8 * 3 = 24 hours
const unsigned long PUMP_RUN_DURATION = 20 * 1000L;        // 60/3=20 seconds
const unsigned long PUMP_RUN_INTERVAL_TEST = 60 * 1000L;   // 60 seconds
const unsigned long PUMP_RUN_DURATION_TEST = 12 * 1000L;   // 12 seconds
const char* PUMP_LOG_FILE = "/file/pump.log";

int ledPin = 2;   // d4 LED_BUILTIN
int pumpPin = 4;  // d2 relay

bool wifiInitialized = false;
bool ntpSyncSuccess = false;
unsigned long pumpStartedMs = 0;
unsigned long pumpLastModifiedMs = 0;

unsigned long pumpLastOnAt = 0;
unsigned long pumpLastOffAt = 0;
unsigned long pumpLastOnElapsed = 0;
unsigned int pumpTotalCounter = 1;
unsigned long pumpTotalElapsed = 0;

String cmdOutput;

BlynkTimer timer;
// libraries/ESP8266WiFi/src/ESP8266WiFiType.h
WiFiEventHandler wh1, wh2, wh3;

String listFiles();
void webSerialRecv(uint8_t* data, size_t len);
void fileLog(const String& text, bool);
void fileLog(const String& text);
String processor(const String& var);
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
NTPClient ntp(ntpUDP, ntpServer, 3600L * 8, 10 * 60 * 1000L);

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
  size_t sz = writeLog(PUMP_LOG_FILE, message);
  Serial.printf("[Log] '%s' (%u bytes) <W>\n", message.c_str(), sz);
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
  Serial.print("[NTP] Current:");
  Serial.print(ntp.getFormattedTime());
  Serial.print(", Epoch:");
  Serial.print(ntp.getEpochTime());
  Serial.print(", Millis: ");
  Serial.println(millis());
  ntpSyncSuccess = true;
  return ntp.getEpochTime();
}

void ntpUpdate() {
  ntpSync();
  if (timeStatus() != timeSet) {
    setTime(ntp.getEpochTime());
  }
}

void statusReport() {
  static int srCount = 0;
  String data = "text=";
  data += urlencode("ESP8266_Status_Report_");
  data += urlencode(WiFi.hostname());
  data += urlencode("_");
  data += (++srCount);
  data += "&desp=";
  data += urlencode("ESP8266 is running");
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
  data += urlencode(" (No.");
  data += srCount;
  data += ")";
  httpsPost(reportUrl, data);
  Serial.printf("[Server] Status Report No.%2d is sent to server.\n", srCount);
}

void pumpReport() {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    pumpTotalCounter++;
  } else {
    pumpTotalElapsed += pumpLastOnElapsed;
  }
  Serial.print("[Server] Pump is ");
  Serial.print(isOn ? "Started" : "Stopped");
  Serial.print(" at ");
  Serial.println(nowString());
  String data = "text=";
  data += urlencode("Pump_Status_Report_");
  data += urlencode(WiFi.hostname());
  data += isOn ? "_Started_" : "_Stopped_";
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
  //   httpsPost(reportUrl, data);
}

void onWiFiConnected(const WiFiEventStationModeConnected& evt) {
    // see onWiFiGotIP
}

void onWiFiDisconnected(const WiFiEventStationModeDisconnected& evt) {
  Serial.print("[WiFi] not connected, ssid: ");
  Serial.print(evt.ssid);
  Serial.print(", reason: ");
  Serial.println(getWiFiDisconnectReason(evt.reason));
}

void onWiFiGotIP(const WiFiEventStationModeGotIP& evt) {
    String msg = "[WiFi] connected, ip: ";
    msg += WiFi.localIP().toString();
    msg += " ssid: ";
    msg += WiFi.SSID();
    Serial.println(msg);
  if (wifiInitialized) {
    fileLog(msg);
  }
}

String processor(const String& var) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (var == "DATE_TIME") {
    return nowString();
  } else if (var == "PUMP_LABEL") {
    return isOn ? "Running" : "Idle";
  } else if (var == "PUMP_ACTION") {
    return isOn ? "off" : "on";
  } else if (var == "PUMP_SUBMIT") {
    return isOn ? "Stop Pump" : "Start Pump";
  } else if (var == "PUMP_LAST_RUN_AT") {
    return pumpLastOnAt > 0 ? dateString(pumpLastOnAt) : "N/A";
  } else if (var == "PUMP_LAST_RUN_DURATION") {
    return pumpLastOnElapsed > 0 ? String(pumpLastOnElapsed) + "s" : "N/A";
  } else if (var == "PUMP_LOGS") {
    String lines = readLog(PUMP_LOG_FILE);
    Serial.printf("[Log] '%s' (%u bytes)<R>\n", "", lines.length());
    lines.replace("\n", "</td></tr>\n<tr><td>");
    lines.replace("\r", "");
    String logText = "<tr><td>";
    logText += lines;
    logText += "</td></tr>";
    Serial.printf("Free Stack 4: %d, Free Heap: %d\n", ESP.getFreeContStack(),
                  ESP.getFreeHeap());
    return logText;
  } else if (var == "CMD_OUTPUT") {
    String output = "";
    if (cmdOutput) {
      output += "<p>";
      cmdOutput.replace("\n", "</p><p>");
      cmdOutput.replace("\r", "");
      output += cmdOutput;
      output += "</p>";
    }
    cmdOutput = "";
    return output;
  }
  return String();
}

void handleNotFound(AsyncWebServerRequest* request) {
  Serial.println("[Server] handleNotFound");
  request->send(404, "text/plain", "Not found");
}

void handleLogs(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleLogs");
  req->send(SPIFFS, "/logs.html", String(), false, processor);
  Serial.printf("[Server] Free Stack: %d, Free Heap: %d\n",
                ESP.getFreeContStack(), ESP.getFreeHeap());
}

void handleRoot(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleRoot");
  //   req->send_P(200, "text/html", ROOT_PAGE_TPL, processor);
  req->send(SPIFFS, "/index.html", String(), false, processor);
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
  timer.setTimeout(PUMP_RUN_DURATION_TEST, stopPump);
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
  req->send(SPIFFS, "/cmd.html", String(), false, processor);
  Serial.printf("[Server] Free Stack: %d, Free Heap: %d\n",
                ESP.getFreeContStack(), ESP.getFreeHeap());
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
  WebSerial.begin(&server);
  WebSerial.msgCallback(webSerialRecv);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/pump", HTTP_POST, handlePump);
  server.on("/clear", HTTP_POST, handleClear);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/cmd", HTTP_GET, handleCmdGet);
  server.on("/cmd", HTTP_POST, handleCmdPost);
  server.serveStatic("/file/", SPIFFS, "/file/");
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[Server] HTTP server started");
}

void webSerialRecv(uint8_t* data, size_t len) {
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setAutoReconnect(true);
  wh1 = WiFi.onStationModeConnected(&onWiFiConnected);
  wh2 = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
  wh3 = WiFi.onStationModeGotIP(&onWiFiGotIP);
  Serial.println("[WiFi] Connecting......");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print("[WiFi] Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print(", IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(", Hostname: ");
  Serial.println(WiFi.hostname());
  wifiInitialized = true;
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
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
  Serial.print(", Time: ");
  Serial.println(timeString());
  if (!WiFi.isConnected()) {
    Serial.println("[WiFi] connection lost, reconnecting...");
    WiFi.reconnect();
  } else {
    // statusReport();
    if (!ntpSyncSuccess) {
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

void setup() {
  // Debug console
  Serial.begin(115200);
  delay(100);
  Serial.setDebugOutput(false);

  Serial.println("\r\n");
  Serial.println("[System] ESP8266 Board is booting...");
  if (!SPIFFS.begin()) {
    Serial.println("[System] Failed to mount file system");
  }
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  setupWiFi();
  setupServer();
  ntpSetup();
  ntpUpdate();

  Blynk.begin(blynkAuth, ssid, pass);
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  timer.setInterval(5 * 60 * 1000L, checkWiFi);  // in milliseconds
  //   timer.setInterval(30 * 60 * 1000L, ntpUpdate);  // in milliseconds
  //   timer.setInterval(5 * 60 * 1000L, statusReport);
  timer.setInterval(PUMP_RUN_INTERVAL_TEST, startPump);
  setSyncInterval(3 * 24 * 60 * 60);  // in seconds = 3 days
  setSyncProvider(ntpSync);
  statusReport();
  fileLog("ESP8266 Board initialized.");
  //   listFiles();
}

void loop() {
  Blynk.run();
  timer.run();
}

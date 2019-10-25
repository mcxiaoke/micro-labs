//#define DEBUG_MODE 1
#define ENABLE_LOGGING 1
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <NTPClient.h>
#include <Updater.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
// #include <user_interface.h>
#include "common/config.h"
#include "common/data.h"
#include "common/internal.h"
#include "common/net.h"
#include "libs/SimpleTimer.h"

const char* JSON_CONFIG_FILE PROGMEM = "/file/config.json";
const char* STATUS_FILE PROGMEM = "/file/status.json";

int ledPin = 2;    // d4 2
int pumpPin = D2;  // d2 io4

typedef struct {
  bool globalSwitchOn;
  unsigned long runInterval;       // ms
  unsigned long runDuration;       // ms
  unsigned long updateInterval;    // ms
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

int runTimerId, wifiTimerId, watchdogTimerId;

SimpleTimer timer;
ESP8266WiFiMulti wifiMgr;
WiFiEventHandler wh1, wh2, wh3;

void saveStatus();
void pumpReport();
String getStatusJson(bool);
String getStatusText();
String listFiles();
void showESP();
void fileLog(const String& text);
void loadConfig();
void showPumpTaskInfo();
void handleNotFound(AsyncWebServerRequest* req);
void handleRoot(AsyncWebServerRequest* req);
void handlePump(AsyncWebServerRequest* req);
void handleSwitch(AsyncWebServerRequest* req);
void handlePump0(bool action);
void startPump();
void stopPump();
void pumpWatchDog();

void apiStart(AsyncWebServerRequest* req);
void apiStop(AsyncWebServerRequest* req);
void apiOn(AsyncWebServerRequest* req);
void apiOff(AsyncWebServerRequest* req);
void apiReboot(AsyncWebServerRequest* req);
void apiSetTimer(AsyncWebServerRequest* req);
void apiDeleteFile(AsyncWebServerRequest* req);
void apiClearLogs(AsyncWebServerRequest* req);

void apiGetStatus(AsyncWebServerRequest* req);
void apiGetLogs(AsyncWebServerRequest* req);
void apiGetFiles(AsyncWebServerRequest* req);
void apiViewFile(AsyncWebServerRequest* req);

void handleOTARedirect(AsyncWebServerRequest* req);
void handleOTAUpdate(AsyncWebServerRequest*,
                     const String&,
                     size_t,
                     uint8_t*,
                     size_t,
                     bool);

const char* ntpServer PROGMEM = "ntp.ntsc.ac.cn";
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, ntpServer, 0, 60 * 60 * 1000L);
AsyncWebServer server(80);

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
    LOGN(F("[Log] Write status file ok."));
  } else {
    LOGN(F("[Log] Write status file failed."));
  }
}

void loadConfig() {
  File file = SPIFFS.open(STATUS_FILE, "r");
  StaticJsonDocument<64> doc;
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
  } else {
    LOGN(msg);
  }
}

void handleNotFound(AsyncWebServerRequest* request) {
  LOGN("[Server] handleNotFound url: " + request->url());
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
  } else {
    request->send(404);
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

void handleRoot(AsyncWebServerRequest* req) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  LOG(F("[Server] handleRoot status="));
  LOGN(isOn ? "On" : "Off");
  req->redirect("/www/");
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
  bool isOn = digitalRead(pumpPin) == 1;
  LOGF("[Watchdog] pump lastRunAt: %s, now: %s status: %s\n",
       timeString(status.lastOnAt).c_str(), timeString().c_str(),
       isOn ? "On" : "Off");
  showESP();
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
  doc["updatePeriod"] = config.updateInterval;
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
  String desp = "Global=";
  desp += config.globalSwitchOn ? "On" : "Off";
  desp += ("\nIP=");
  desp += (WiFi.localIP().toString());
  desp += ("\nSSID=");
  desp += (WiFi.SSID());
  desp += ("\nWiFi Status=");
  desp += (WiFi.status());
  desp += "\nDateTime=";
  desp += nowString();
  desp += ("\nUpTime=");
  desp += humanTimeMs(millis());
  desp += ("\nlastRunAt2=");
  desp += (dateTimeString(status.lastOnAt));
  desp += ("\nlastRunAt=");
  desp += (dateTimeString(now() - timer.getElapsed(runTimerId) / 1000));
  desp += ("\nnextRunAt=");
  desp += dateTimeString(now() + timer.getRemain(runTimerId) / 1000);
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

void handleRemoteGetStatusJson(AsyncWebServerRequest* req) {
  req->send(200, "application/json", getStatusJson(true));
}

void handleRemoteGetStatusText(AsyncWebServerRequest* req) {
  String text = getStatusText();
  text.replace(", ", "\n");
  req->send(200, "text/plain", text);
}

void handleOTARedirect(AsyncWebServerRequest* req) {
  req->redirect("/www/update.html");
}

void handleOTAUpdate(AsyncWebServerRequest* request,
                     const String& filename,
                     size_t index,
                     uint8_t* data,
                     size_t len,
                     bool final) {
  if (!index) {
    LOGN("[OTA] Update begin, file: " + filename);
    size_t conentLength = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    Update.runAsync(true);
    if (!Update.begin(conentLength, cmd, ledPin, LOW)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  } else {
    if (progressPrintMs == 0 || millis() - progressPrintMs > 500) {
      LOGF("[OTA] Upload progress: %d%%\n",
           (Update.progress() * 100) / Update.size());
      progressPrintMs = millis();
    }
  }

  if (final) {
    request->send(200, "text/plain", "ok");
    delay(1000);
    if (!Update.end(true)) {
      Update.printError(Serial);
    } else {
      setOTAUpdated();
      fileLog("[OTA] Board firmware update completed.");
      ESP.restart();
    }
  }
}

//////////
void apiStart(AsyncWebServerRequest* req) {
  startPump();
//   req->send(200, "text/plain", "ok");
  req->redirect("/");
}
void apiStop(AsyncWebServerRequest* req) {
  stopPump();
  req->redirect("/");
}
void apiOn(AsyncWebServerRequest* req) {
  config.globalSwitchOn = true;
  saveStatus();
  req->redirect("/");
}
void apiOff(AsyncWebServerRequest* req) {
  config.globalSwitchOn = false;
  saveStatus();
  req->redirect("/");
}
void apiReboot(AsyncWebServerRequest* req) {
  LOG(F("[Server] Pump is rebooting..."));
  req->redirect("/");
  server.end();
  ESP.restart();
}

void apiSetTimer(AsyncWebServerRequest* req) {
  LOG(F("[Server] Set Timer"));
  if (runTimerId > -1) {
    timer.deleteTimer(runTimerId);
  }
  runTimerId = timer.setInterval(config.runInterval, startPump);
  req->redirect("/");
}

void apiDeleteFile(AsyncWebServerRequest* req) {
  LOGN(F("[Server] handleRemoteDeleteFile"));
  String output = "";
  if (req->hasParam("file_path", true)) {
    AsyncWebParameter* path = req->getParam("file_path", true);
    String filePath = path->value();
    if (SPIFFS.remove(filePath)) {
      output = "File:";
      output += filePath;
      output += " is deleted.";
      req->send(200, "text/plain", output);
    } else {
      output = "Failed to delete file:";
      output += filePath;
      req->send(404, "text/plain", output);
    }
  } else {
    req->send(400, "text/plain", "Invalid Parameters.");
  }
}

void apiClearLogs(AsyncWebServerRequest* req) {
  bool removed = SPIFFS.remove(logFileName());
  LOG(F("[Server] Pump logs cleared, result: "));
  LOGN(removed ? "success" : "fail");
  File f = SPIFFS.open(logFileName(), "w");
  if (f) {
    f.write((uint8_t)0x00);
    f.close();
  }
  req->redirect("/");
}

void apiGetStatus(AsyncWebServerRequest* req) {
  LOG(F("[Server] apiGetStatus "));
  LOGN(req->getHeader("Accept")->toString());
  bool useJson =
      String("application/json")
          .equalsIgnoreCase(req->getHeader("Accept")->value()) ||
      (req->hasParam("format") &&
       String("json").equalsIgnoreCase(req->getParam("format")->value()));
  if (useJson) {
    req->send(200, F("application/json"), getStatusJson(true));
  } else {
    String text = getStatusText();
    text.replace(", ", "\n");
    req->send(200, "text/plain", text);
  }
}
void apiGetLogs(AsyncWebServerRequest* req) {
  LOGN(F("[Server] apiGetLogs"));
  if (SPIFFS.exists(logFileName())) {
    req->send(SPIFFS, logFileName(), "text/plain");
  } else {
    req->send(200, "text/plain", "");
  }
}
void apiGetFiles(AsyncWebServerRequest* req) {
  LOGN(F("[Server] handleRemoteGetFiles"));
  String content = listFiles();
  req->send(200, "text/plain", content);
}

void apiViewFile(AsyncWebServerRequest* req) {
  LOGN(F("[Server] handleRemoteEditFile"));
  size_t hc = req->headers();
  for (size_t i = 0; i < hc; i++) {
    LOGF("[Header] %s : %s\n", req->headerName(i).c_str(),
         req->header(i).c_str());
  }
  AsyncWebParameter* path = req->getParam("file-path", true);
  AsyncWebParameter* data = req->getParam("file-data", true);
  LOGF("file-path=");
  LOGN(path->value());
  LOGF("file-data=");
  LOGN(data->value());
  req->send(200, "text/plain", "ok");
}
//////////

void setupApi() {
  server.on("/api/reboot", HTTP_POST, apiReboot);
  server.on("/api/timer", HTTP_POST, apiSetTimer);
  server.on("/api/start", HTTP_POST, apiStart);
  server.on("/api/stop", HTTP_POST, apiStop);
  server.on("/api/on", HTTP_POST, apiOn);
  server.on("/api/off", HTTP_POST, apiOff);
  server.on("/api/clear_logs", HTTP_POST, apiClearLogs);
  server.on("/api/delete_file", HTTP_POST, apiDeleteFile);

  server.on("/api/files", HTTP_GET, apiGetFiles);
  server.on("/api/logs", HTTP_GET, apiGetLogs);
  server.on("/api/view", HTTP_POST, apiViewFile);
  server.on("/api/status", HTTP_GET, apiGetStatus);
}

void setupServer() {
  if (MDNS.begin("esp8266")) {  // Start the mDNS responder for esp8266.local
    LOGN(F("[Server] mDNS responder started"));
  } else {
    LOGN(F("[Server] Error setting up MDNS responder!"));
  }

  // accessible at "<IP Address>/webserial" in browser
  //   WebSerial.begin(&server);
  //   WebSerial.msgCallback(webSerialRecv);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.serveStatic("/file/", SPIFFS, "/file/");
  server.serveStatic("/www/", SPIFFS, "/www/")
      .setLastModified(now() - millis() / 1000)
      .setCacheControl("max-age=600")
      .setDefaultFile("index.html");
  server.on("/update", HTTP_GET, handleOTARedirect);
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest* request) {},
            [](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
              handleOTAUpdate(request, filename, index, data, len, final);
            });
  setupApi();
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), "*");
  DefaultHeaders::Instance().addHeader("Server", WiFi.hostname());
#ifdef DEBUG_MODE
  DefaultHeaders::Instance().addHeader("DebugMode", "True");
#endif
  server.begin();
  LOGN(F("[Server] HTTP server started"));
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
  wifiMgr.addAP(ap1_ssid, ap1_pass);
  wifiMgr.addAP(ap2_ssid, ap2_pass);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  WiFi.setAutoReconnect(true);
  wh1 = WiFi.onStationModeConnected(&onWiFiConnected);
  wh2 = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
  wh3 = WiFi.onStationModeGotIP(&onWiFiGotIP);
  LOGN(F("[WiFi] Connecting......"));
  int retry = 100;
  while (wifiMgr.run() != WL_CONNECTED && --retry > 0) {
    delay(500);
  }
  if (wifiMgr.run() != WL_CONNECTED) {
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
  config.updateInterval = 5 * 60 * 1000L;            // 5min
  config.watchDogInterval = config.runDuration / 2;  // 7s;
  config.ntpInterval = 8 * 60 * 1000L;               // 8min
  config.wifiInterval = 2 * 60 * 1000L;              // 2min
#else
  debugMode = false;
  config.runInterval = 8 * 3600 * 1000L;             // 8 * 3 = 24 hours
  config.runDuration = 20 * 1000L;                   // 60/3=20 seconds
  config.updateInterval = 4 * 3600 * 1000L;          // 4hours
  config.watchDogInterval = config.runDuration / 2;  // 10s
  config.ntpInterval = 2 * 3600 * 1000L;             // 2 hours
  config.wifiInterval = 30 * 60 * 1000L;             // 30min
#endif
  LOGF("[System] setupData, debug=%s\n", debugMode ? "True" : "False");
}

void setupTimers() {
  LOGN(F("[System] Setup timers."));
  setSyncInterval(config.ntpInterval / 1000);  // in seconds
  setSyncProvider(ntpSync);
  //   timer.setInterval(30 * 60 * 1000L, ntpUpdate);  // in milliseconds
  wifiTimerId =
      timer.setInterval(config.wifiInterval, checkWiFi);  // in milliseconds
  runTimerId = timer.setInterval(config.runInterval, startPump);
  watchdogTimerId = timer.setInterval(config.watchDogInterval, pumpWatchDog);
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
  setupServer();
  setupTimers();
  saveStatus();
  fileLog("[System] Board boot on " + nowString());
  showESP();
  LOGN(F("============= SETUP END ============="));
}

void loop() {
  timer.run();
}

void pumpReport() {
  LOGN(F("[Server] Pump report."));
  //   showESP();
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    status.totalCounter++;
  } else {
    status.totalElapsed += status.lastOnElapsed;
    LOGN("[Server] Pump is scheduled at " +
         dateTimeString(now() + config.runInterval / 1000));
  }
  bool debugMode = false;
#ifdef DEBUG_MODE
  debugMode = true;
#endif
  LOGF("[Report] Pump is %s at %s debugMode: %s\n",
       isOn ? "Started" : "Stopped", nowString().c_str(),
       (debugMode ? "True" : "False"));
  String desp = "";
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
  //   desp += ", Next:";
  //   desp += dateTimeString(now() + timer.getRemain(runTimerId) / 1000);
  //   desp += ", IP:";
  //   desp += WiFi.localIP().toString();
  desp = "";
  saveStatus();
#ifndef DEBUG_MODE
  //   String data = "text=";
  //   data += urlencode(F("Pump"));
  //   data += urlencode(isOn ? F("_Started") : F("_Stopped"));
  //   data += F("&desp=");
  //   data += urlencode(isOn ? F(" Started at ") : F(" Stopped at "));
  //   data += urlencode(timeString(isOn ? status.lastOnAt : status.lastOffAt));
  //   httpsPost(reportUrl, data);
  //   data = "";
  //   LOGF("[Report] %s Pump action report is sent to server. \n",
  //        WiFi.hostname().c_str());
  showESP();
#endif
}

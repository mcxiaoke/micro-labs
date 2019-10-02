#define DEBUG_MODE 1
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Updater.h>
#include <time.h>
// #include <WebSerial.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
// #include <user_interface.h>

#include "common/config.h"
#include "common/data.h"
#include "common/internal.h"
#include "common/net.h"
#include "libs/SimpleTimer.h"

// static esp8266::polledTimeout::periodicMs showTimeNow(10000);
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

const char* PUMP_LOG_FILE = "/file/pump.log";
const char* JSON_CONFIG_FILE = "/file/config.json";
const char* STATUS_FILE = "/file/status.json";

int ledPin = 2;    // d4 2
int pumpPin = D2;  // d2 io4

unsigned long runInterval;     // ms
unsigned long runDuration;     // ms
unsigned long updateInterval;  // ms
unsigned long wifiInterval;    // ms
unsigned long ntpInterval;     // ms

bool wifiInitialized = false;
bool ntpInitialized = false;
bool globalSwitchOn = true;
unsigned long progressPrintMs;
unsigned long pumpStartedMs = 0;
unsigned long pumpLastModifiedMs = 0;

unsigned long pumpLastOnAt = 0;
unsigned long pumpLastOffAt = 0;
unsigned long pumpLastOnElapsed = 0;
unsigned int pumpTotalCounter = 0;
unsigned long pumpTotalElapsed = 0;
unsigned long ntpLastRunAt = 0;

int runTimerId, wifiTimerId, updateTimerId;

SimpleTimer timer;
WiFiEventHandler wh1, wh2, wh3;

void blinkLED();
void fileStatus();
void statusReport();
void pumpReport();
String getStatusJson(bool);
String getStatusText();
String listFiles();
void webSerialRecv(uint8_t* data, size_t len);
void handleWebSerialCmd(const String& cmd);
void fileLog(const String& text, bool);
void fileLog(const String& text);
void saveConfig();
void loadConfig();
void showPumpTaskInfo();
String templateProcessor(const String& var);
void handleNotFound(AsyncWebServerRequest* req);
void handleRoot(AsyncWebServerRequest* req);
void handlePump(AsyncWebServerRequest* req);
void handleSwitch(AsyncWebServerRequest* req);
void handlePump0(bool action);
void startPump();
void stopPump();
void handleClearLogs(AsyncWebServerRequest* req);
void handleResetBoard(AsyncWebServerRequest* req);

void handleRemoteDeleteFile(AsyncWebServerRequest* req);
void handleRemoteGetFiles(AsyncWebServerRequest* req);
void handleRemoteGetLogs(AsyncWebServerRequest* req);
void handleRemoteGetStatusJson(AsyncWebServerRequest* req);
void handleRemoteGetStatusText(AsyncWebServerRequest* req);
void handleOTARedirect(AsyncWebServerRequest* req);
void handleOTAUpdate(AsyncWebServerRequest*,
                     const String&,
                     size_t,
                     uint8_t*,
                     size_t,
                     bool);

const char* ntpServer = "ntp.ntsc.ac.cn";
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, ntpServer, 0, 60 * 60 * 1000L);
AsyncWebServer server(80);

void blinkLED() {
  digitalWrite(ledPin, !digitalRead(ledPin));
}

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
  message += " [";
  message += globalSwitchOn ? "On" : "Off";
  message += "]";
  size_t c = writeLog(PUMP_LOG_FILE, message);
  if (c) {
    Serial.printf("[Log] %u bytes log written to file.\n", c);
  }
}

void fileStatus() {
  File file = SPIFFS.open(STATUS_FILE, "w");
  if (file) {
    // file.println("========== BEGIN ==========");
    // file.print(status);
    // file.println("=========== END ===========");
    // file.print("Created At: ");
    file.println(getStatusJson(true));
    file.close();
    // file.println();
    Serial.println(F("[Log] Write status file ok."));
  } else {
    Serial.println(F("[Log] Write status file failed."));
  }
}

void saveConfig() {
  StaticJsonDocument<64> doc;
  doc["switch"] = globalSwitchOn;
  doc["ts"] = now();
  //   SPIFFS.remove(JSON_CONFIG_FILE);
  File file = SPIFFS.open(JSON_CONFIG_FILE, "w");
  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("[Config] Failed to save json config."));
  }
  Serial.println(F("[Config] saveConfig: "));
  serializeJsonPretty(doc, Serial);
  Serial.println();
  file.close();
}

void loadConfig() {
  File file = SPIFFS.open(JSON_CONFIG_FILE, "r");
  StaticJsonDocument<64> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("[Config] Failed to load json config."));
  }
  Serial.println(F("[Config] loadConfig: "));
  serializeJsonPretty(doc, Serial);
  Serial.println();
  globalSwitchOn = doc["switch"] | true;
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
  Serial.print(F("[NTP] Server time is "));
  Serial.println(dateTimeString(ntp.getEpochTime()));
  if (ntp.getEpochTime() < TIME_START) {
    // invalid time, failed, retry after 5s
    // timer.setTimeout(5 * 1000L, ntpSync);
    Serial.println(F("[NTP] Server time invalid."));
    return 0;
  }
  ntpLastRunAt = ntp.getEpochTime();
  Serial.print("[NTP] Synced, Curent Time:");
  Serial.print(dateTimeString(ntp.getEpochTime()));
  Serial.print(", Up Time: ");
  Serial.println(humanTimeMs(millis()));
  ntpInitialized = true;
  return ntp.getEpochTime();
}

void ntpUpdate() {
  ntpSync();
  if (ntp.getEpochTime() > TIME_START) {
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

String templateProcessor(const String& var) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (var == "DEBUG") {
#ifdef DEBUG_MODE
    return "(DEBUG)";
#else
    return "";
#endif
  } else if (var == "DATE_TIME") {
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
    return humanTimeMs(runInterval);
  } else if (var == "PUMP_DURATION") {
    return humanTimeMs(runDuration);
  } else if (var == "PUMP_NEXT_AT") {
    return dateTimeString(now() + timer.getRemain(runTimerId) / 1000);
  } else if (var == "PUMP_NEXT_REMAINS") {
      return humanTimeMs(timer.getRemain(runTimerId));
  } else if (var == "SWITCH_LABEL") {
    return globalSwitchOn ? "On" : "Off";
  } else if (var == "SWITCH_SUBMIT") {
    return globalSwitchOn ? "Switch Off" : "Switch On";
  } else if (var == "SWITCH_ACTION") {
    return globalSwitchOn ? "off" : "on";
  }
  return String();
}

void handleNotFound(AsyncWebServerRequest* request) {
  Serial.println("[Server] handleNotFound url: " + request->url());
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
  } else {
    request->send(404);
  }
}

void showPumpTaskInfo() {
  Serial.printf("[Pump] Now time is %s\n", nowString().c_str());
  Serial.printf(
      "[Pump] Last run at %s, elapsed %s\n",
      dateTimeString(now() - timer.getElapsed(runTimerId) / 1000).c_str(),
      humanTimeMs(timer.getElapsed(runTimerId)).c_str());
  Serial.printf(
      "[Pump] Next run at %s, remain %s\n",
      dateTimeString(now() + timer.getRemain(runTimerId) / 1000).c_str(),
      humanTimeMs(timer.getRemain(runTimerId)).c_str());
  Serial.flush();
}

void handleRoot(AsyncWebServerRequest* req) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  Serial.print("[Server] handleRoot status=");
  Serial.println(isOn ? "On" : "Off");
  //   req->send_P(200, "text/html", ROOT_PAGE_TPL, processor);
  req->send(SPIFFS, "/www/index.html", String(), false, templateProcessor);
  showPumpTaskInfo();
}

void handlePump0(bool action) {
  if (!globalSwitchOn) {
    Serial.print(F("[Pump] handlePump0 global switch is off, ignore."));
    return;
  }
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
  Serial.print("[Pump] Pump is ");
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
  Serial.println("[Pump] startPump at " + nowString());
  handlePump0(true);
  timer.setTimeout(runDuration, stopPump);
}
void stopPump() {
  Serial.println("[Pump] stopPump at " + nowString());
  handlePump0(false);
}

void handlePump(AsyncWebServerRequest* req) {
  if (pumpLastModifiedMs > 0 && millis() - pumpLastModifiedMs < 2000L) {
    Serial.println("[Pump] Pump action too often, ignored.");
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
  Serial.printf("[Pump] Pump actionStr=%s, isOn=%d\n", actionStr.c_str(), isOn);
  handlePump0(action);
  req->redirect("/");
}

void handleSwitch(AsyncWebServerRequest* req) {
  if (!req->hasArg("action")) {
    req->redirect("/");
    return;
  }
  String actionStr = req->arg("action");
  bool action = actionStr == "on";
  bool isOn = globalSwitchOn;
  Serial.printf("[Server] Switch actionStr=%s, isOn=%d\n", actionStr.c_str(),
                isOn);
  globalSwitchOn = action;
  req->redirect("/");
  saveConfig();
}

void handleClearLogs(AsyncWebServerRequest* req) {
  bool removed = SPIFFS.remove(PUMP_LOG_FILE);
  Serial.print("[Server] Pump logs cleared, result: ");
  Serial.println(removed ? "success" : "fail");
  req->redirect("/");
}

void handleResetBoard(AsyncWebServerRequest* req) {
  Serial.print("[Server] Pump is rebooting...");
  req->redirect("/");
  server.end();
  ESP.restart();
}

void handleRemoteDeleteFile(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleRemoteDeleteFile");
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

void handleRemoteGetFiles(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleRemoteGetFiles");
  String content = listFiles();
  req->send(200, "text/plain", content);
}
void handleRemoteGetLogs(AsyncWebServerRequest* req) {
  Serial.println("[Server] handleRemoteGetLogs");
  req->send(SPIFFS, PUMP_LOG_FILE, "text/plain");
}

String getStatusJson(bool pretty) {
  StaticJsonDocument<512> doc;
  doc["ts"] = now();
  doc["name"] = WiFi.hostname();
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  //   doc["mac"] = WiFi.macAddress();
  doc["on"] = digitalRead(pumpPin) == HIGH;
  doc["lastElapsed"] = pumpLastOnElapsed;
  // timer or manual last run at
  doc["lastAt2"] = pumpLastOnAt;
  // timer task last run at
  doc["lastAt"] = now() - timer.getElapsed(runTimerId) / 1000;
  doc["nextAt"] = now() + timer.getRemain(runTimerId) / 1000;
  doc["interval"] = runInterval;
  doc["duration"] = runDuration;
  doc["wifiPeriod"] = wifiInterval;
  doc["updatePeriod"] = updateInterval;
  doc["ntpPeriod"] = ntpInterval;
  doc["ntpLast"] = ntpLastRunAt;
  doc["switch"] = globalSwitchOn;

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
  String desp = "Board:";
  desp += WiFi.hostname();
#ifdef DEBUG_MODE
  desp += ", Debug Mode=true";
#endif
  desp += ", Global Switch=";
  desp += globalSwitchOn ? "On" : "Off";
  desp += (", IP=");
  desp += (WiFi.localIP().toString());
  desp += (", SSID=");
  desp += (WiFi.SSID());
  desp += (", WiFi Status=");
  desp += (WiFi.status());
  desp += ", DateTime=";
  desp += nowString();
  desp += (", UpTime=");
  desp += humanTimeMs(millis());
  desp += (", lastRunAt2=");
  desp += (dateTimeString(pumpLastOnAt));
  desp += (", lastRunAt=");
  desp += (dateTimeString(now() - timer.getElapsed(runTimerId) / 1000));
  desp += (", nextRunAt=");
  desp += dateTimeString(now() + timer.getRemain(runTimerId) / 1000);
  desp += (", lastOnElapsed=");
  desp += humanTime(pumpLastOnElapsed);
  desp += (", totalElapsed=");
  desp += pumpTotalElapsed;
  desp += (", runInterval=");
  desp += humanTimeMs(runInterval);
  desp += (", runDuration=");
  desp += humanTimeMs(runDuration);
  desp += (", wifiInterval=");
  desp += humanTimeMs(wifiInterval);
  desp += (", ntpInterval=");
  desp += humanTime(ntpInterval);
  desp += (", ntpLastRunAt=");
  desp += dateTimeString(ntpLastRunAt);
  desp += ", Free Stack=";
  desp += ESP.getFreeContStack();
  desp += ", Free Heap=";
  desp += ESP.getFreeHeap();
  return desp;
}

void handleRemoteGetStatusJson(AsyncWebServerRequest* req) {
  //   Serial.printf("[System] Free Stack: %d, Free Heap: %d\n",
  //                 ESP.getFreeContStack(), ESP.getFreeHeap());
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
    Serial.println("[OTA] Update begin, file: " + filename);
    size_t conentLength = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    Update.runAsync(true);
    if (!Update.begin(conentLength, cmd)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  } else {
    if (progressPrintMs == 0 || millis() - progressPrintMs > 500) {
      Serial.printf("[OTA] Upload progress: %d%%\n",
                    (Update.progress() * 100) / Update.size());
      progressPrintMs = millis();
    }
  }

  if (final) {
    AsyncWebServerResponse* response = request->beginResponse(
        302, "text/plain", "[OTA] Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)) {
      Update.printError(Serial);
    } else {
      setOTAUpdated();
      fileLog("[OTA] Board firmware update completed.");
      Serial.println("[OTA] Update complete, will reboot.");
      Serial.flush();
      ESP.restart();
    }
  }
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
  server.onNotFound(handleNotFound);
  server.serveStatic("/file/", SPIFFS, "/file/");
  server.serveStatic("/www/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.on("/j/toggle_pump", HTTP_POST, handlePump);
  server.on("/j/toggle_switch", HTTP_POST, handleSwitch);
  server.on("/j/clear_logs", HTTP_POST, handleClearLogs);
  server.on("/j/reset_board", HTTP_POST, handleResetBoard);
  server.on("/j/delete_file", HTTP_POST, handleRemoteDeleteFile);
  server.on("/j/get_files", HTTP_GET, handleRemoteGetFiles);
  server.on("/j/get_logs", HTTP_GET, handleRemoteGetLogs);
  server.on("/j/get_status_json", HTTP_GET, handleRemoteGetStatusJson);
  server.on("/j/get_status_text", HTTP_GET, handleRemoteGetStatusText);
  server.on("/ota", HTTP_GET, handleOTARedirect);
  server.on("/www/update.html", HTTP_POST,
            [](AsyncWebServerRequest* request) {},
            [](AsyncWebServerRequest* request, const String& filename,
               size_t index, uint8_t* data, size_t len, bool final) {
              handleOTAUpdate(request, filename, index, data, len, final);
            });
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Server", WiFi.hostname());
#ifdef DEBUG_MODE
  DefaultHeaders::Instance().addHeader("DebugMode", "True");
#endif
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

void setupNTP() {
  //   configTime(8 * 3600L, 0, "ntp.ntsc.ac.cn", "cn.ntp.org.cn",
  //              "cn.pool.ntp.org");
  ntp.begin();
  int retry = 100;
  Serial.println("[NTP] Sync, get time from server.");
  ntpUpdate();
  while (timeStatus() != timeSet && --retry > 0) {
    delay(1000);
    Serial.println("[NTP] Sync, get time failed, retry");
    ntpUpdate();
  }
  if (timeStatus() != timeSet) {
    // ntp udpate failed, reboot
    ESP.restart();
  }
  Serial.println(nowString());
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
  fileStatus();
  if (!WiFi.isConnected()) {
    Serial.println("[WiFi] connection lost, reconnecting...");
    WiFi.reconnect();
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
  runInterval = 3 * 60 * 1000L;     // 3min
  runDuration = 35 * 1000L;         // 35s
  updateInterval = 5 * 60 * 1000L;  // 5min
  ntpInterval = 8 * 60 * 1000L;     // 8min
  wifiInterval = 2 * 60 * 1000L;    // 2min
#else
  debugMode = false;
  runInterval = 8 * 3600 * 1000L;     // 8 * 3 = 24 hours
  runDuration = 20 * 1000L;           // 60/3=20 seconds
  updateInterval = 4 * 3600 * 1000L;  // 4hours
  ntpInterval = 2 * 3600 * 1000L;     // 2 hours
  wifiInterval = 10 * 60 * 1000L;     // 10min
#endif
  Serial.printf("[System] setupData, debugMode=%s\n",
                debugMode ? "True" : "False");
}

void setupTimers() {
  Serial.println(F("[System] Setup timers."));
  setSyncInterval(ntpInterval / 1000);  // in seconds = 1 days
  setSyncProvider(ntpSync);
  //   timer.setInterval(30 * 60 * 1000L, ntpUpdate);  // in milliseconds
  //   timer.setInterval(5 * 60 * 1000L, statusReport);
  wifiTimerId = timer.setInterval(wifiInterval, checkWiFi);  // in milliseconds
  runTimerId = timer.setInterval(runInterval, startPump);
  updateTimerId = timer.setInterval(updateInterval, statusReport);
}

void setup() {
  // Debug console
  // Free Stack:2352, Free Heap:35928
  Serial.begin(115200);
  Serial.println();
  delay(1000);
  Serial.println("[System] ESP8266 Board is booting...");
  EEPROM.begin(1024);
  if (!SPIFFS.begin()) {
    Serial.println("[System] Failed to mount file system");
  } else {
    Serial.println("[System] SPIFFS file system mounted.");
  }
  if (isOTAUpdated()) {
    Serial.println("[System] OTA Updated! " + ESP.getSketchMD5());
  }

  loadConfig();
  setupData();
  setupWiFi();
  setupNTP();
  setupServer();
  setupTimers();
  saveConfig();
  listFiles();
  statusReport();
  fileLog("[System] ESP8266 Board powered on at " + nowString());
}

void loop() {
  timer.run();
}

void statusReport() {
  static int srCount = 0;
  String data = "text=";
  data += urlencode("Pump_");
  data += urlencode(WiFi.hostname());
  data += urlencode("_Status_");
#ifdef DEBUG_MODE
  data += urlencode("DEBUG_");
#endif
  data += (++srCount);
  String desp = getStatusText();
  data += "&desp=";
  data += urlencode(desp);
  desp.replace(",", "\n");
//   Serial.println(desp);
#ifndef DEBUG_MODE
  httpsPost(reportUrl, data);
  Serial.printf("[Report] %s Status Report No.%2d is sent to server.\n",
                WiFi.hostname().c_str(), srCount);
#endif
}

void pumpReport() {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    pumpTotalCounter++;
  } else {
    pumpTotalElapsed += pumpLastOnElapsed;
    Serial.println("[Server] Pump is scheduled at " +
                   dateTimeString(now() + runInterval / 1000));
  }
  bool debugMode = false;
#ifdef DEBUG_MODE
  debugMode = true;
#endif
  Serial.printf("[Report] Pump is %s at %s debugMode: %s\n",
                isOn ? "Started" : "Stopped", nowString().c_str(),
                (debugMode ? "True" : "False"));

  String data = "text=";
  data += urlencode("Pump_");
  data += urlencode(WiFi.hostname());
  data += urlencode(isOn ? "_Started" : "_Stopped");
  data += urlencode(debugMode ? "_DEBUG_" : "_");
  data += pumpTotalCounter;
  data += "&desp=";
  String desp = "Pump";
  desp += isOn ? " Started" : " Stopped";
  desp += " at ";
  desp += timeString();
  desp += ", IP: ";
  desp += WiFi.localIP().toString();
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
  desp += ") ";
  fileLog(desp);
  desp += getStatusText();
  data += urlencode(desp);
#ifndef DEBUG_MODE
  httpsPost(reportUrl, data);
  Serial.printf("[Report] %s Pump Report is sent to server.\n",
                WiFi.hostname().c_str());
#endif
}

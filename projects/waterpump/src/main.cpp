#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <FS.h>
#define BLYNK_PRINT Serial
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <PageBuilder.h>
#include <TimeLib.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include "config.h"
#include "data.h"
#include "internal.h"
#include "net.h"

// 1 minutes per day, 2 times, 30 seconds per one run
const unsigned long PUMP_RUN_INTERVAL = 8 * 60 * 60 * 1000L;  // 8 hours
const unsigned long PUMP_RUN_DURATION = 20 * 1000L;            // 60/3=20 seconds
const unsigned long PUMP_RUN_INTERVAL_TEST = 1 * 60 * 1000L;   // 1 minutes
const unsigned long PUMP_RUN_DURATION_TEST = 10 * 1000L;       // 10 seconds
const char* PUMP_LOG_FILENAME = "pump.log";

int ledPin = 2;   // d4 LED_BUILTIN
int pumpPin = 4;  // d2 relay

bool ntpSyncSuccess = false;
unsigned long pumpStartedMs = 0;
unsigned long pumpLastModifiedMs = 0;

unsigned long pumpLastOnAt = 0;
unsigned long pumpLastOffAt = 0;
unsigned long pumpLastOnElapsed = 0;
unsigned int pumpTotalCounter = 1;
unsigned long pumpTotalElapsed = 0;

BlynkTimer timer;
// libraries/ESP8266WiFi/src/ESP8266WiFiType.h
WiFiEventHandler wh1, wh2, wh3;

void pumpLog(const String& text, bool);
void pumpLog(const String& text);
void handleNotFound();
void handleLED0(bool action);
void handleLED();
void startLED();
void stopLED();
void handlePump();
void handlePump0(bool action);
void startPump();
void stopPump();
void handleClear();
void handleReset();
void redirectRoot();

const char* ntpServer = "ntp.ntsc.ac.cn";
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, ntpServer, 3600L * 8, 10 * 60 * 1000L);

ESP8266WebServer server(80);

void pumpLog(const String& text) {
  pumpLog(text, true);
}

void pumpLog(const String& text, bool appendDate) {
  String message = "";
  if (appendDate) {
    message += "[";
    message += nowString();
    message += "] ";
  }
  message += text;
  Serial.print("[System] Write Log: ");
  Serial.println(message);
  writeLog(PUMP_LOG_FILENAME, message);
}

String getDateTimeLabel(PageArgument& args) {
  return nowString();
}

String getLedLabel(PageArgument& args) {
  bool isOn = digitalRead(ledPin) == LOW;
  return isOn ? "Off" : "On";
}

String getLedAction(PageArgument& args) {
  bool isOn = digitalRead(ledPin) == LOW;
  return isOn ? "off" : "on";
}

String getLedSubmit(PageArgument& args) {
  bool isOn = digitalRead(ledPin) == LOW;
  return isOn ? "Turn Off LED" : "Turn On LED";
}

String getPumpLabel(PageArgument& args) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  return isOn ? "Running" : "Idle";
}

String getPumpAction(PageArgument& args) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  return isOn ? "off" : "on";
}

String getPumpSubmit(PageArgument& args) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  return isOn ? "Stop Pump" : "Start Pump";
}

String getPumpLastRunAt(PageArgument& args) {
  return pumpLastOnAt > 0 ? dateString(pumpLastOnAt) : "N/A";
}

String getPumpLastRunDuration(PageArgument& args) {
  return pumpLastOnElapsed > 0 ? String(pumpLastOnElapsed) + "s" : "N/A";
}

String getLogPageLines(PageArgument& args) {
  String log = readLog(PUMP_LOG_FILENAME);
  log.replace("\r\n", "</td></tr>\n            <tr><td>");
  return "<tr><td>" + log + "</td></tr>";
}

PageElement LogsElement(LOGS_PAGE_TPL,
                        {
                            {"PUMP_LOGS", getLogPageLines},
                        });
PageBuilder LogsPage("/logs", {LogsElement});
PageElement RootElement(ROOT_PAGE_TPL,
                        {
                            {"DATE_TIME", getDateTimeLabel},
                            {"LED_LABEL", getLedLabel},
                            {"LED_ACTION", getLedAction},
                            {"LED_SUBMIT", getLedSubmit},
                            {"PUMP_LABEL", getPumpLabel},
                            {"PUMP_ACTION", getPumpAction},
                            {"PUMP_SUBMIT", getPumpSubmit},
                            {"PUMP_LAST_RUN_AT", getPumpLastRunAt},
                            {"PUMP_LAST_RUN_DURATION", getPumpLastRunDuration},
                        });
PageBuilder RootPage("/", {RootElement});

void ntpSetup() {
  ntp.begin();
}

time_t ntpSync() {
  ntp.update();
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
  data += urlencode("ESP8266-Status-Report-");
  data += (++srCount);
  data += "&desp=";
  data += urlencode("ESP8266 is Online");
  data += urlencode(", Current Time: ");
  data += urlencode(nowString());
  data += urlencode(", Up Time: ");
  data += millis() / 1000;
  data += urlencode("s, Timestamp: ");
  data += now();
  data += urlencode(", IP:");
  data += urlencode(WiFi.localIP().toString());
  data += urlencode(" (No.");
  data += srCount;
  data += ")";
  httpsPost(reportUrl, data);
  Serial.printf("[Server] Status Report No.%2d is sent to server.\n", srCount);
}

void onWiFiConnected(const WiFiEventStationModeConnected& evt) {
  handleLED0(true);
  Serial.print("[WiFi] connected, ssid: ");
  Serial.println(evt.ssid);
  pumpLog("WiFi Connected to " + evt.ssid);
}

void onWiFiDisconnected(const WiFiEventStationModeDisconnected& evt) {
  handleLED0(false);
  Serial.print("[WiFi] not connected, ssid: ");
  Serial.print(evt.ssid);
  Serial.print(", reason: ");
  Serial.println(getWiFiDisconnectReason(evt.reason));
}

void onWiFiGotIP(const WiFiEventStationModeGotIP& evt) {
  handleLED0(true);
  Serial.print("[WiFi] got ip: ");
  Serial.println(evt.ip);
  pumpLog("WiFi got ip " + evt.ip.toString());
}

void handleNotFound() {
  Serial.println("[Server] handleNotFound");
  server.send(404, "text/plain", "404: Not found");
}

void handleLED0(bool action) {
  // pin2 is special, low is on, high is off
  digitalWrite(ledPin, action ? LOW : HIGH);
  bool isOn = digitalRead(ledPin) == LOW;
  Serial.print("[Server] LED is ");
  Serial.print(isOn ? "On" : "Off");
  Serial.print(" at ");
  Serial.println(nowString());
}

void startLED() {
  Serial.println("[CMD] startLED");
  handleLED0(true);
  timer.setTimeout(PUMP_RUN_DURATION_TEST, stopLED);
}

void stopLED() {
  Serial.println("[CMD] stopLED");
  handleLED0(false);
}

void handleLED() {
  String actionStr = server.arg("action");
  bool action = actionStr == "on";
  Serial.print("[Server] handleLED action=");
  Serial.println(actionStr);
  handleLED0(action);
  redirectRoot();
}

void pumpReport() {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (isOn) {
    pumpTotalCounter++;
  } else {
    pumpTotalElapsed += pumpLastOnElapsed;
  }
  Serial.print("[Server] Pump is ");
  Serial.println(isOn ? "Started" : "Stopped");
  String data = "text=";
  data += urlencode("Pump-Status-Report-");
  data += pumpTotalCounter;
  data += "&desp=";
  String desp = " Pump";
  desp += isOn ? " Started" : " Stopped";
  desp += " at ";
  desp += timeString();
  desp += ", Total: ";
  desp += pumpTotalElapsed;
  desp += "s";
  if (!isOn) {
    desp += ", Current: ";
    desp += pumpLastOnElapsed;
    desp += "s";
  }
  desp += " (";
  desp += pumpTotalCounter;
  desp += ")";
  pumpLog(desp);
  data += urlencode(desp);
    httpsPost(reportUrl, data);
}

void redirectRoot() {
  server.sendHeader("Location", "/");
  server.send(303);
}

void handlePump0(bool action) {
  bool isOn = digitalRead(pumpPin) == HIGH;
  if (action == isOn) {
    redirectRoot();
    return;
  }
  unsigned long nowTs = now();
  unsigned long elapsed = 0;
  if (pumpStartedMs > 0) {
    // in seconds
    elapsed = (millis() - pumpStartedMs + 1.0f) / 1000.0;
  }
  Serial.print("[Server] Pump is ");
  Serial.print(isOn ? "Stopping" : "Starting");
  if (isOn) {
    Serial.print(", Elapsed Time: ");
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
  Serial.print(" at ");
  Serial.println(timeString());
  digitalWrite(pumpPin, action ? HIGH : LOW);
  pumpLastModifiedMs = millis();
  timer.setTimeout(200L, pumpReport);
}

void startPump() {
  Serial.println("[CMD] startPump");
  handlePump0(true);
  timer.setTimeout(PUMP_RUN_DURATION, stopPump);
}
void stopPump() {
  Serial.println("[CMD] stopPump");
  handlePump0(false);
}

void handlePump() {
  if (pumpLastModifiedMs > 0 && millis() - pumpLastModifiedMs < 2000L) {
    Serial.println("[Server] Pump action too often, ignored.");
    redirectRoot();
    return;
  }
  String actionStr = server.arg("action");
  bool action = actionStr == "on";
  bool isOn = digitalRead(pumpPin) == HIGH;
  Serial.printf("[Server] Pump actionStr=%s, isOn=%d\n", actionStr.c_str(),
                isOn);
  handlePump0(action);
  redirectRoot();
}

void handleClear() {
  bool removed = SPIFFS.remove(PUMP_LOG_FILENAME);
  Serial.print("[Server] Pump logs cleared, result: ");
  Serial.println(removed ? "success" : "fail");
  redirectRoot();
}

void handleReset() {
  Serial.print("[Server] Pump is rebooting...");
  redirectRoot();
  ESP.restart();
}

void setupServer() {
  if (MDNS.begin("esp8266")) {  // Start the mDNS responder for esp8266.local
    Serial.println("[Server] mDNS responder started");
  } else {
    Serial.println("[Server] Error setting up MDNS responder!");
  }
  //   server.on("/", handleRoot);
  RootPage.insert(server);
  LogsPage.insert(server);
  server.on("/led", HTTP_POST, handleLED);
  server.on("/pump", HTTP_POST, handlePump);
  server.on("/clear", HTTP_POST, handleClear);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[Server] HTTP server started");
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setAutoReconnect(true);
  wh1 = WiFi.onStationModeConnected(&onWiFiConnected);
  wh2 = WiFi.onStationModeDisconnected(&onWiFiDisconnected);
  wh3 = WiFi.onStationModeGotIP(&onWiFiGotIP);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  // If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("[WiFi] Connected to ");
  Serial.println(ssid);
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());  // IP address assigned to your ESP
  Serial.print("[WiFi] Hostname: ");
  Serial.println(WiFi.hostname());
}

// for led pin
BLYNK_WRITE(V0)  // Button Widget is writing to pin V0
{
  int v = param.asInt();
  Serial.printf("[Blynk] value %d received at V0\n", v);
  handleLED0(v == HIGH);
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

BLYNK_APP_CONNECTED() {
  Serial.println("[Blynk] App Connected!");
}

BLYNK_APP_DISCONNECTED() {
  Serial.println("[Blynk] App Disconnected!");
}

void checkWiFi() {
  Serial.print("[WiFi] Status: ");
  Serial.print(WiFi.status());
  Serial.print(", IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(", Time: ");
  Serial.println(timeString());
  //   Serial.println(ntp.getFormattedTime());
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

  pumpLog("ESP8266 Borad booting...");

  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  setupWiFi();
  setupServer();
  ntpSetup();
  ntpUpdate();
  statusReport();
  Blynk.begin(blynkAuth, ssid, pass);
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  timer.setInterval(30 * 1000L, checkWiFi);  // in milliseconds
  //   timer.setInterval(30 * 60 * 1000L, ntpUpdate);  // in milliseconds
  //   timer.setInterval(5 * 60 * 1000L, statusReport);
  timer.setInterval(PUMP_RUN_INTERVAL, startPump);
  setSyncInterval(24 * 60 * 60);  // in seconds = 1 hour
  setSyncProvider(ntpSync);
  //   SPIFFS.remove("config.log");
}

void loop() {
  Blynk.run();
  timer.run();
  server.handleClient();
}

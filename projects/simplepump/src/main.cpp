#define DEBUG_MODE
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WiFiClient.h>
#include "ESPTime.h"
#include "SimpleTimer.h"
#include "config.h"

#ifdef DEBUG_MODE
const unsigned long RUN_INTERVAL = 60 * 1000L;
const unsigned long RUN_DURATION = 18 * 1000L;
#else
const unsigned long RUN_INTERVAL = 8 * 3600 * 1000L;
const unsigned long RUN_DURATION = 20 * 1000L;
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const int led = 2;
const int pump = D2;

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

void startPump();
void stopPump();
void checkPump();
void checkWiFi();

SimpleTimer timer;
ESP8266WebServer server(80);

void showESP() {
  Serial.printf("[System] Free Stack: %d, Free Heap: %d\n",
                ESP.getFreeContStack(), ESP.getFreeHeap());
}

size_t debugLog(const String& text) {
  String msg = "[";
  msg += dateTimeString();
  msg += "] ";
  msg += text;
  Serial.println(msg);
  File f = SPIFFS.open("/file/log.txt", "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(msg);
  c += f.print('\n');
  f.close();
  return c;
}

void startPump() {
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  debugLog("Pump started");
  timer.setTimeout(RUN_DURATION, stopPump);
}

void stopPump() {
  lastStop = millis();
  if (lastStart > 0) {
    lastSeconds = (lastStop - lastStart) / 1000;
    totalSeconds += lastSeconds;
  }
  digitalWrite(pump, LOW);
  digitalWrite(led, HIGH);
  debugLog("Pump stopped");
}

void checkPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn && lastStart > 0 && (millis() - lastStart) / 1000 >= RUN_DURATION) {
    debugLog("Pump stopped by watchdog");
    stopPump();
  }
  showESP();
}

void checkWiFi() {
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog("WiFi reconnect");
  }
}

void handleLog() {
  File f = SPIFFS.open("/file/log.txt", "r");
  if (!f) {
    server.send(200, "text/plain", "404 NOT FOUND");
  } else {
    server.send(200, "text/plain", f.readString());
  }
}

void handleRoot() {
  unsigned long ts = millis();
  String data = "Pump Status\n";
  data += "System Boot: ";
  data += formatDateTime(getTimestamp() - ts / 1000);
  data += "\n";
  if (lastStart > 0) {
    data += "Last Start: ";
    data += formatDateTime(getTimestamp() - (ts - lastStart) / 1000);
    data += "\n";
  }
  if (lastStart > 0) {
    data += "Last Stop: ";
    data += formatDateTime(getTimestamp() - (ts - lastStop) / 1000);
    data += "\n";
  }
  data += "Last Elapsed: ";
  data += lastSeconds;
  data += "s\n";
  data += "Total Elapsed: ";
  data += totalSeconds;
  data += "s\n";
  server.send(200, "text/plain", data.c_str());
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  debugLog("WiFi Connecting");
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 30 * 1000L) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  digitalWrite(led, LOW);
  Serial.begin(115200);
  setupWiFi();
  initESPTime();
  if (MDNS.begin("esp8266")) {
    debugLog("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/api/status", handleRoot);
  server.on("/start", startPump);
  server.on("/stop", stopPump);
  server.begin();
  debugLog("HTTP server started");
  digitalWrite(led, HIGH);
  timer.setInterval(RUN_INTERVAL, startPump);
  timer.setInterval(RUN_DURATION / 2 + 1000, checkPump);
  timer.setInterval(10 * 60 * 1000L, checkWiFi);
  debugLog("System initialized");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  timer.run();
}

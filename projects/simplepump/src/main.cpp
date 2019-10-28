//#define DEBUG_MODE
#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WiFiClient.h>
#include "ESPTime.h"
#include "SimpleTimer.h"
#include "config.h"

#undef LED_BUILTIN
#define LED_BUILTIN 2

#ifdef DEBUG_MODE
const unsigned long RUN_INTERVAL = 60 * 1000L;
const unsigned long RUN_DURATION = 18 * 1000L;
#else
const unsigned long RUN_INTERVAL = 6 * 3600 * 1000L;
const unsigned long RUN_DURATION = 15 * 1000L;
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const int led = LED_BUILTIN;
const int pump = D2;

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

int runTimerId;

static const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";

void startPump();
void stopPump();
void checkPump();
void checkWiFi();

SimpleTimer timer;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

void showESP() {
  Serial.printf("Free Stack: %d, Free Heap: %d\n", ESP.getFreeContStack(),
                ESP.getFreeHeap());
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

void startPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  debugLog("Pump started");
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
  debugLog("Pump stopped");
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

void handleRoot() {
  unsigned long ts = millis();
  String data = "";
  data += "System Boot: ";
  data += formatDateTime(getTimestamp() - ts / 1000);
  data += "\n";
  data += "Next Start: ";
  data += formatDateTime(getTimestamp() + timer.getRemain(runTimerId) / 1000);
  data += "\n";
  if (lastStart > 0) {
    data += "Last Start: ";
    data += formatDateTime(getTimestamp() - (ts - lastStart) / 1000);
    data += "\n";
  }
  if (lastStop > 0) {
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
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(led, HIGH);
}

void setupTime() {
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
  timer.setInterval(10 * 60 * 1000L, checkWiFi);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  if (!SPIFFS.begin()) {
    Serial.println(F("Failed to mount file system"));
  } else {
    Serial.println(F("SPIFFS file system mounted."));
  }
  setupWiFi();
  setupTime();
  setupServer();
  setupTimers();
  debugLog(F("System initialized"));
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  timer.run();
}

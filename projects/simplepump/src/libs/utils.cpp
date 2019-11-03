#include "utils.h"

static time_t upTimestamp = 0;  // in seconds

// void _log(const char* format, ...) {
// #if defined(EANBLE_LOGGING) || defined(DEBUG_MODE)
//   char buffer[256];
//   va_list args;
//   va_start(args, format);
//   vsprintf(buffer, format, args);
//   va_end(args);
//   Serial.print(buffer);
// #endif
// }

String listFiles() {
  String output = "";
  Serial.println(F("[System] SPIFFS Files:"));
#if defined(ESP8266)

  Dir f = SPIFFS.openDir("/");
  while (f.next()) {
    Serial.printf("[File] %s (%d bytes)\n", f.fileName().c_str(), f.fileSize());
    output += f.fileName();
    output += "\n";
  };

#elif defined(ESP32)
  File root = SPIFFS.open("/");
  if (root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      Serial.printf("[File] %s (%d bytes)\n", f.name(), f.size());
      output += f.name();
      // output += " (";
      // output += f.fileSize();
      // output += ")";
      output += "\n";
      f = root.openNextFile();
    };
  }

#endif
  return output;
}

void fsCheck() {
#if defined(ESP8266)
  if (!SPIFFS.begin()) {
#elif defined(ESP32)
  if (!SPIFFS.begin(true)) {
#endif
    Serial.println(F("[System] Failed to mount file system"));
  } else {
    Serial.println(F("[System] SPIFFS file system mounted."));
  }
}

String getDevice() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac.substring(mac.length() / 2);
}

String getMD5(uint8_t* data, uint16_t len) {
  MD5Builder md5;
  md5.begin();
  md5.add(data, len);
  md5.calculate();
  return md5.toString();
}

String getMD5(const char* data) {
  MD5Builder md5;
  md5.begin();
  md5.add(data);
  md5.calculate();
  return md5.toString();
}

String getMD5(const String& data) {
  return getMD5(data.c_str());
}

void showESP() {
#if defined(ESP8266)
  Serial.printf("[System] Free Stack: %d, Free Heap: %d\n",
                ESP.getFreeContStack(), ESP.getFreeHeap());
#elif defined(ESP32)
  Serial.printf("[System] Free Heap: %d\n", ESP.getFreeHeap());
#endif
}

String logFileName() {
  String fileName = "/file/log-";
  fileName += dateString();
  fileName += ".txt";
  return fileName;
}

size_t fileLog(const String& text, bool appendDate) {
  String message = "";
  if (appendDate) {
    message += "[";
    message += timeString();
    message += "] ";
  }
  message += text;
  String fileName = logFileName();
  size_t c = writeLog(fileName, message);
  message = "";
  fileName = "";
  return c;
}

size_t writeLog(const String& path, const String& text) {
  File f = SPIFFS.open(path, "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(text);
  c += f.print('\n');
  f.close();
  return c;
}

String readLog(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return "";
  }
  String s = f.readString();
  f.close();
  return s;
}

void setTimestamp() {
  auto timeOut = 3000U;
  auto time = getNtpTime(timeOut);
  auto startMs = millis();
  while (time < TIME_START_2019 && (millis() - startMs) < 30 * 1000L) {
    time = getNtpTime(timeOut);
  }
  if (time > TIME_START_2019) {
    upTimestamp = time - millis() / 1000;
    Serial.print("[NTP] Time: ");
    Serial.println(dateTimeString());
  } else {
    Serial.println("[NTP] time failed\n");
  }
}

time_t getTimestamp() {
  return upTimestamp + millis() / 1000;
}

String dateString() {
  time_t ts = getTimestamp();
  char buf[12];
  struct tm* tm_info;
  tm_info = localtime(&ts);
  strftime(buf, 12, "%Y-%m-%d", tm_info);
  return String(buf);
}

String dateTimeString() {
  return formatDateTime(getTimestamp());
}

String timeString() {
  return formatTime(getTimestamp());
}

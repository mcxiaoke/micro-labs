#ifndef __UTILS_H__
#define __UTILS_H__

#include <MD5Builder.h>
#include "compat.h"
#include "ntp.h"
#include "tools.h"

#if defined(EANBLE_LOGGING) || defined(DEBUG_MODE)
#define LOG(...) Serial.print(__VA_ARGS__)
#define LOGN(...) Serial.println(__VA_ARGS__)
#define LOGF(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#define LOGN(...)
#define LOGF(...)
#endif

String listFiles();
void fsCheck();
String getDevice();
String getMD5(uint8_t* data, uint16_t len);
String getMD5(const char* data);
String getMD5(const String& data);
void showESP();
String logFileName();
size_t fileLog(const String& text, bool appendDate);
size_t writeLog(const String& path, const String& text);
String readLog(const String& path);

void setTimestamp();
time_t getTimestamp();
String dateString();
String dateTimeString();
String timeString();

#endif
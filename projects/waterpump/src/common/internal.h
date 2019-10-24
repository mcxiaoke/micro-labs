#ifndef _INTERNAL_H
#define _INTERNAL_H

#include <Arduino.h>
#include <EEPROM.h>
#include <FS.h>
#include <TimeLib.h>
#include <time.h>

#ifdef ENABLE_LOGGING
#define LOG(...) Serial.print(__VA_ARGS__)
#define LOGN(...) Serial.println(__VA_ARGS__)
#define LOGF(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#define LOGN(...)
#define LOGF(...)
#endif

#define LEAP_YEAR(Y)                        \
  (((1970 + Y) > 0) && !((1970 + Y) % 4) && \
   (((1970 + Y) % 100) || !((1970 + Y) % 400)))

const time_t TIME_START = 1500000000L;  // in seconds
const time_t TZ_OFFSET = 8 * 3600L;     // in seconds

String logFileName();
size_t fileLog(const String& text, bool appendDate);
size_t writeLog(const String& path, const String& text);
String readLog(const String& path);
int timedRead(Stream& s);
size_t countLine(const String& path);   // with yield, only in loop
size_t countLine2(const String& path);  // without yield, use in anyhwere
String nowStringGMT();
String nowString();
String dateString();
String timeString();
String dateString(unsigned long ts);
String timeString(unsigned long ts);
String dateTimeString(unsigned long ts);
String gmtString(unsigned long ts);
String humanTimeMs(unsigned long ts);
String humanTime(unsigned long ts);
void eSetup();
void eWriteInt(int address, int value);
int eReadInt(int address);
void htmlEscape(String& html);
String URLEncode(const char* msg);
String urldecode(String str);
String urlencode(String str);
unsigned char h2int(char c);

#endif
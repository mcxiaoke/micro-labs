#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include <Arduino.h>
#include <time.h>

bool strEqual(const char* str1, const char* str2);
String humanTimeMs(unsigned long ts);
String humanTime(unsigned long ts);
String formatDateTime(time_t ts);
String formatTime(time_t ts);
void getDateTimeStr(time_t, char*);
void getTimeStr(time_t, char*);
void htmlEscape(String& html);
String URLEncode(const char* msg);
String urldecode(String str);
String urlencode(String str);
unsigned char h2int(char c);

#endif
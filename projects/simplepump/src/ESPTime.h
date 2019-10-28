#ifndef __ESP_TIME_H__
#define __ESP_TIME_H__

#include <ESP8266WiFi.h>
#include <time.h>

void initESPTime();
time_t getTimestamp();
String dateTimeString();
String formatDateTime(time_t ts);
String timeString();
String formatTime(time_t ts);
void getDateTimeStr(time_t, char*);
void getTimeStr(time_t, char*);

#endif
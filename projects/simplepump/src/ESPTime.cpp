#include "ESPTime.h"
#include "ntp.h"

static time_t upTimestamp = 0;  // in seconds

void initESPTime() {
  unsigned int timeOut = 5000;
  time_t time = getNtpTime(timeOut);
  unsigned long startMs = millis();
  while (time < TIME_START && (millis() - startMs) < 30 * 1000L) {
    time = getNtpTime(timeOut);
  }
  if (time > TIME_START) {
    upTimestamp = time - millis() / 1000;
    Serial.print("NTP Time: ");
    Serial.println(dateTimeString());
  } else {
    Serial.println("NTP time failed\n");
  }
}

time_t getTimestamp() {
  return upTimestamp + millis() / 1000;
}

String formatDateTime(time_t ts) {
  char buf[26];
  getDateTimeStr(ts, buf);
  return String(buf);
}

String dateTimeString() {
  return formatDateTime(getTimestamp());
}

String formatTime(time_t ts) {
  char buf[12];
  getTimeStr(ts, buf);
  return String(buf);
}

String timeString() {
  return formatTime(getTimestamp());
}

void getDateTimeStr(time_t ts, char* buffer) {
  struct tm* tm_info;
  tm_info = localtime(&ts);
  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

void getTimeStr(time_t ts, char* buffer) {
  struct tm* tm_info;
  tm_info = localtime(&ts);
  strftime(buffer, 12, "%H:%M:%S", tm_info);
}

#ifndef __NTP__H__
#define __NTP__H__

#include <WiFiUdp.h>
#include <time.h>
#include "compat.h"

#ifndef TIME_ZONE_OFFSET
#define TIME_ZONE_OFFSET (8)
#endif

#ifndef SECS_PER_HOUR
#define SECS_PER_MIN (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
// the time at the start of y2k
#define SECS_YR_2000 (946684800UL)
// time seconds between 1900-1970 unix
#define SECS_DELTA_1900_1970 (2208988800UL)
#endif

const time_t TIME_START_2019 = 1500000000L;  // in seconds

void sendNTPpacket(IPAddress& address);
time_t getNtpTime(unsigned int timeOut);
int getNtpTimeZone();

#endif
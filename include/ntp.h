#pragma once

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

#define TZ  "CST6CDT,M3.2.0,M11.1.0" // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define NTP_SERVER0 "0.us.pool.ntp.org"
#define NTP_SERVER1 "1.us.pool.ntp.org"
#define NTP_SERVER2 "2.us.pool.ntp.org"
#define SERIAL_PORT Serial
//#define TS (timeinfo.tm_year < (2020-1900)) ? millis(): tv.tv_sec
#define TS (tv.tv_sec==NULL) ? millis() : tv.tv_sec


void doNtp(void *);
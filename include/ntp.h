#pragma once

#include "global.h"
#include <esp_sntp.h>
#include <time.h>

#define UTCOFFSETSEC 18000
#define DSTOFFSETSEC 3600
#define TZ  "CST6CDT,M3.2.0,M11.1.0" // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define NTP_SERVER0 "0.us.pool.ntp.org"
#define NTP_SERVER1 "1.us.pool.ntp.org"
#define NTP_SERVER2 "2.us.pool.ntp.org"

void doNtp(void *);
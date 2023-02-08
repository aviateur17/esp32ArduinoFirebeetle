#pragma once
#include <Arduino.h>
#include "hostdefs.h"
#include "credentials.h"
#include "TelnetSpy.h"


//#define SERIAL_PORT Serial
#define SERIAL_PORT ts
#define LED GPIO_NUM_2
#define NEOPIN GPIO_NUM_25
#define PIRPIN GPIO_NUM_34
#define SDAPIN GPIO_NUM_21
#define SCLPIN GPIO_NUM_22
#define BTNPIN GPIO_NUM_35
#define LEDON 1
#define LEDOFF 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)
#define TEMPOFFSET 0
#define HUMOFFSET 0
#define PRESSOFFSET 0
#define MQTTHBINTERVALMS 30000uL
#define APP_VERSION "0,0,4"
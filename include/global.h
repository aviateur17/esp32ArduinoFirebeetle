#pragma once
#include <Arduino.h>
#include <esp_log.h>
#include "hostdefs.h"
#include "credentials.h"

#define WIFI
#define TEMP
#define SLEEP
//#define ESPNOW
// #define BLESERVER
//#define BLECLIENT


#ifdef WIFI
#define SERIAL_PORT Serial
#else
#define SERIAL_PORT Serial
#endif

#define LED GPIO_NUM_2
// #define NEOPIN GPIO_NUM_25
// #define PIRPIN GPIO_NUM_34
// #define SDAPIN GPIO_NUM_21
// #define SCLPIN GPIO_NUM_22
// #define BTNPIN GPIO_NUM_35
#define TEMPINTPIN GPIO_NUM_34
#define TEMPEXTPIN GPIO_NUM_39
#define BATTVPIN GPIO_NUM_36
#define ADCOFFSET 1
#define LEDON 1 
#define LEDOFF 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
// #define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)
#define TS millis()
#define TEMPOFFSET 0
#define HUMOFFSET 0
#define PRESSOFFSET 0
#define MQTTHBINTERVALMS 30000uL
#define APP_VERSION "0,0,5"
#define PROGNAME "DFRobotESP32TempsBattProtoBoard"

#ifdef WIFI
#undef ESPNOW
#endif
#ifdef BLESERVER
#undef BLECLIENT
#endif
#pragma once

#include <Arduino.h>
#include "jclwifi.h"
#include "ntp.h"
#include "jclmqtt.h"
#include "esp_ota.h"
#include <Esp.h> // ESP32 chip information
#include <Preferences.h> // NVS

// Use only core 1
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t pro_cpu = 0;
  static const BaseType_t app_cpu = 1;
#endif


#define SERIAL_PORT Serial
#define LED GPIO_NUM_2
#define PIRPIN GPIO_NUM_34
#define LEDON 1
#define LEDOFF 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define HOSTNAME
#define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)
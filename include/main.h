#pragma once

#include "global.h"
#ifdef WIFI
#include "jclwifi.h"            // Wifi
#include "ntp.h"                // NTP
#include "jclmqtt.h"            // MQTT
#include "esp_ota.h"            // OTA
#endif // END WIFI
#include <Esp.h>                // ESP32 chip information
#include <Preferences.h>        // NVS
#ifdef BLESERVER
#include "jclble.h"
#endif

// Use only core 1
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t pro_cpu = 0;
  static const BaseType_t app_cpu = 1;
#endif
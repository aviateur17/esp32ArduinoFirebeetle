#pragma once

#include "global.h"
#include <WiFi.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

void doInitWifiSta(void *);
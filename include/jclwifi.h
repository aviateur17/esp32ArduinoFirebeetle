#pragma once

#include <WiFi.h>
#include "credentials.h"
#include "hostdefs.h"
//#include "TelnetSpy.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 5
#define SERIAL_PORT Serial
//#define SERIAL_PORT ts

void doInitWifiSta(void *);
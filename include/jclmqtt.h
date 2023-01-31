#pragma once

#include "PubSubClient.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include "hostdefs.h"
#include "credentials.h"
//#include "TelnetSpy.h"

// See hostdefs.h for more MQTT definitions
// MQTT_CLIENTID
// MQTT_TOPIC
#define MQTTHBINTERVALMS 30000uL
#define APP_VERSION "0,0,4"

#define SERIAL_PORT Serial

void doMqtt(void *);
#pragma once

#include "global.h"
#include "PubSubClient.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <string>
#include <sys/time.h>
#include "ArduinoJson-v6.20.0.h"

void doMqtt(void *);
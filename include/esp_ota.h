#pragma once

#include "ArduinoOTA.h"
#include "hostdefs.h"

#define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)

void doOta(void *);
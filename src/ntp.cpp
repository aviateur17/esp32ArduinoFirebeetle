#include "ntp.h"

extern time_t now;
extern char strftime_buf[64];
extern struct tm timeinfo;
extern struct timeval tv;
extern SemaphoreHandle_t semWifi;
extern TelnetSpy ts;
static const char TAG[] = __FILE__;

void time_sync_notification_cb(struct timeval *tval) {
  SERIAL_PORT.printf("%lu: Time synchronization event.\n", TS);
  return;
}

void doNtp(void * params) {
  unsigned long updateTime = 0;
  unsigned long printDateTime = 0;

  ESP_LOGI(TAG,"NTP TASK STARTING");
  while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
    ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.\n", 5000);
  }
  xSemaphoreGive(semWifi);

  //setenv("TZ", TZ, 1);
  //tzset();
  //configTime(UTCOFFSETSEC, DSTOFFSETSEC, NTP_SERVER0, NTP_SERVER1, NTP_SERVER2);
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, NTP_SERVER0);
  sntp_setservername(1, NTP_SERVER1);
  sntp_setservername(2, NTP_SERVER2);
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);

  time(&now);
  localtime_r(&now, &timeinfo);
  if(timeinfo.tm_year < (2020 - 1900)) {
    SERIAL_PORT.printf("%lu: Time is not yet set.\n", TS);
    sntp_init();
    time(&now);
  }

  if(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
    SERIAL_PORT.printf("%lu: Waiting for system time to be set.\n", TS);
  }
  vTaskDelay(5000/portTICK_PERIOD_MS);
  time(&now);
  localtime_r(&now, &timeinfo);


  tv.tv_sec = now;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  SERIAL_PORT.printf("%lu: The current date/time is: %s\n", TS, strftime_buf);

  while(true) {

    if(millis() - printDateTime > (5 * 60 * 1000)) {
      time(&now);
      localtime_r(&now, &timeinfo);
      if(timeinfo.tm_year < (2020 - 1900)) {
        SERIAL_PORT.printf("%lu: Time is not yet set.\n", TS);
        sntp_init();
        time(&now);
      }

      if(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        SERIAL_PORT.printf("%lu: Waiting for system time to be set.\n", TS);
      }
      time(&now);
      localtime_r(&now, &timeinfo);


      tv.tv_sec = now;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
      SERIAL_PORT.printf("%lu: The current date/time is: %s\n", TS, strftime_buf);

      printDateTime = millis();
    }

    if(millis() - updateTime > 500) {
      time(&now);
      localtime_r(&now, &timeinfo);
      tv.tv_sec = now;
      tv.tv_usec = 0;
      updateTime = millis();
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
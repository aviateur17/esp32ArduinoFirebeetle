#include "esp_ota.h"

extern  SemaphoreHandle_t semWifi;

void doOta(void * param) {
    // Take Wifi semaphore - we depend upon wifi

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  //
  //ArduinoOTA.setHostname("MyESP32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
    ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.", 5000);
  }
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //ESP_LOGI(TAG, "Start updating " + type);
    })
    .onEnd([]() {
      ESP_LOGI(TAG, "\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      ESP_LOGI(TAG, "Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      ESP_LOGE(TAG,"Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) ESP_LOGE(TAG, "Auth Failed");
      else if (error == OTA_BEGIN_ERROR) ESP_LOGE(TAG, "Begin Failed");
      else if (error == OTA_CONNECT_ERROR) ESP_LOGE(TAG, "Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) ESP_LOGE(TAG, "Receive Failed");
      else if (error == OTA_END_ERROR) ESP_LOGE(TAG, "End Failed");
    });
    
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.begin();
    xSemaphoreGive(semWifi);
    ESP_LOGI(TAG, "OTA Started!");  


    while(true) {

        ArduinoOTA.handle();
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);

}
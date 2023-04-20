/*
Code by Jeffrey Lehman

ESP32 DFRobot Firebeetle board testing

See README.md for more information

*/

#include "main.h"
static const char TAG[] = __FILE__;
time_t now;
char strftime_buf[64];
struct tm timeinfo;
struct timeval tv;
int ledBlinks = 2;
float tempF, humPct, presshPa;
bool bmeDetected;
TaskHandle_t tskLedFlash, tskPir, tskBme280, tskBtnPress, tskTelnetSpy, tskInitWifi, tskBleServer;
TaskHandle_t tskNeoMatrix, tskNtp, tskMqtt, tskOta;
SemaphoreHandle_t semWifi;
Preferences nvs;
// BlueDot_BME280 bme;
// TelnetSpy ts;

// initialize LED on GPIO2 as output and Pull down
void initLed(gpio_num_t pin) {
  gpio_config_t gpioLed;
  gpioLed.pin_bit_mask = ((1ULL<< pin));
  gpioLed.mode         = GPIO_MODE_OUTPUT;
  gpioLed.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpioLed.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpioLed.intr_type    = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&gpioLed));
}



void doLedFlash(void * param) {
  int numFlashes = 0;

  ESP_LOGI(TAG,"LED TASK STARTING");

  gpio_set_level(LED, LEDOFF);
  vTaskDelay(88/portTICK_PERIOD_MS);
  gpio_set_level(LED, LEDON);
  vTaskDelay(88/portTICK_PERIOD_MS);
  gpio_set_level(LED, LEDOFF);

  if(param == NULL)
    numFlashes = 0;
  else
    numFlashes = *((int*) param);

  while(true) {

    for(int i = 1; i <= numFlashes; i++) {
      gpio_set_level(LED, LEDOFF);
      vTaskDelay(88/portTICK_PERIOD_MS);
      gpio_set_level(LED, LEDON);
      vTaskDelay(88/portTICK_PERIOD_MS);
      gpio_set_level(LED, LEDOFF);
    }
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }
  ESP_LOGI(TAG,"Deleting LED TASK");
  vTaskDelete(NULL);
}


// void doTelnetSpy(void* param) {
//   while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
//     ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.\n", 5000);
//   }
//   xSemaphoreGive(semWifi);
//   SERIAL_PORT.begin(115200);
//   ESP_LOGI(TAG,"TELNETSPY TASK STARTING");
//   // SERIAL_PORT.setCallbackOnConnect(telnetConnected);
//   // SERIAL_PORT.setCallbackOnDisconnect(telnetDisconnected);

//   while(true) {
//     ts.handle();
    
//     vTaskDelay(1000/portTICK_PERIOD_MS);
//   }
//   vTaskDelete(NULL);
// }

void esp_info() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG,
          "This is ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision "
          "%d, %dMB %s Flash",
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
          chip_info.revision, spi_flash_get_chip_size() / (1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                          : "external");
  // ESP Framework only???
  //ESP_LOGI(TAG, "CPU Clock speed %d", rtc_clk_cpu_freq_get());
  ESP_LOGI(TAG, "Internal Total heap %d, internal Free Heap %d",
          ESP.getHeapSize(), ESP.getFreeHeap());
  #ifdef BOARD_HAS_PSRAM
  ESP_LOGI(TAG, "SPIRam Total heap %d, SPIRam Free Heap %d",
          ESP.getPsramSize(), ESP.getFreePsram());
  #endif
  ESP_LOGI(TAG, "ChipRevision %d, Cpu Freq %d, SDK Version %s",
          ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG, "Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(),
          ESP.getFlashChipSpeed());
  // ESP Framework only???
  //ESP_LOGI(TAG, "Wifi/BT software coexist version %s",
  //        esp_coex_version_get());

}

void setup() {
  esp_reset_reason_t resetReason;

  //Print time in setup in order to benchmark
  SERIAL_PORT.begin(115200);
  delay(5000);
  ESP_LOGI(TAG,"Starting...");
  ESP_LOGI(TAG,"ESP32S3 Seeed Xiao Board");
  ESP_LOGI(TAG,"**** ESP32 S3 Arduno Seeed Xiao board testing. ****");
  
  ESP_LOGE(TAG,"Log Error Message...");       // Lowest log level
  ESP_LOGW(TAG,"Log Warning Message...");
  ESP_LOGI(TAG,"Log Information Message...");
  ESP_LOGD(TAG,"Log Debug Message...");
  ESP_LOGV(TAG,"Log Verbose Message...");     // Highest log level

  resetReason = esp_reset_reason();
  ESP_LOGI(TAG,"Reset Reason: %i",resetReason);
  
  esp_info();
  
  initLed(LED);
  if(nvs.begin("esp32", false))
    ESP_LOGI(TAG,"NVS Initialized.");
  else
    ESP_LOGE(TAG,"Error initializing NVS.");
  ESP_LOGI(TAG,"%u free entries in NVS.",nvs.freeEntries());
  nvs.end(); 

  #ifdef WIFI
  semWifi = xSemaphoreCreateBinary();
  xSemaphoreGive(semWifi);
  xSemaphoreTake(semWifi, portMAX_DELAY);

  // Re priority - higher number is higher priority
  xTaskCreatePinnedToCore(&doInitWifiSta, "InitWifi", 8192, NULL, 0, &tskInitWifi, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doNtp, "NTP", 2048, NULL, 0, &tskNtp, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doMqtt, "Mqtt", 8192, NULL, 0, &tskMqtt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doOta, "OTA", 2048, NULL, 0, &tskOta, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  #endif // END WIFI
  xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, (void *) &ledBlinks, 0, &tskLedFlash, app_cpu);
  // xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, NULL, 0, &tskLedFlash, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doPir, "PIR", 2048, NULL, 0, &tskPir, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doBme280, "BME280", 2048, NULL, 0, &tskBme280, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doBtnPress, "Button", 2048, NULL, 0, &tskBtnPress, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doNeoLoop, "Neo Matrix", 2048, NULL, 0, &tskNeoMatrix, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doBleServer, "BLE Server", 8096, NULL, 0, &tskBleServer, app_cpu);
  // doBleServer(nullptr);
  vTaskDelay(500/portTICK_PERIOD_MS);
}

void loop() {
  // put your main code here, to run repeatedly:
  //LedBlink(2);
  vTaskDelay(2000/portTICK_PERIOD_MS);
}
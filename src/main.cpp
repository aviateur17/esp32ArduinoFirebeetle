/*
ESP32 DFRobot Firebeetle board testing

Todo:
Serial1 output to another board
BLE server/client
BATTERY CHARging?
battery voltage?
battery level?
A/C power?
TelnetSpy?

VBatt on A0 GPIO_NUM_36
voltage divider 1M and 1Meg

*/

#include "main.h"
static const char TAG[] = __FILE__;
time_t now;
char strftime_buf[64];
struct tm timeinfo;
struct timeval tv;
int ledBlinks = 2;
//BaseType_t returncode;
TaskHandle_t tskNtp, tskLedFlash, tskInitWifi, tskMqtt, tskPir, tskOta;
SemaphoreHandle_t semWifi;
Preferences nvs;

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

void initPir(gpio_num_t pin) {
  gpio_config_t gpioPir;
  gpioPir.pin_bit_mask = ((1ULL<< pin));
  gpioPir.mode         = GPIO_MODE_INPUT;
  gpioPir.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpioPir.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioPir.intr_type    = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&gpioPir));
}

void doPir(void * param) {
  ESP_LOGI(TAG,"PIR TASK STARTING");
  bool pirCurrentState = 0;

  pirCurrentState = gpio_get_level(PIRPIN);

  while(true) {
    if(gpio_get_level(PIRPIN) && pirCurrentState == 0) {
      gpio_set_level(LED, LEDON);
      SERIAL_PORT.printf("%lu: PIR High.\n", TS);
      pirCurrentState = 1;
      vTaskDelay(500/portTICK_PERIOD_MS);
      gpio_set_level(LED, LEDON);
    }
    else if(!gpio_get_level(PIRPIN) && pirCurrentState == 1) {
      gpio_set_level(LED, LEDOFF);
      SERIAL_PORT.printf("%lu: PIR Low.\n", TS);
      pirCurrentState = 0;
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
  }
  vTaskDelete(NULL);
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
  //Print time in setup in order to benchmark
  SERIAL_PORT.begin(115200);
  delay(5000);
  ESP_LOGI(TAG,"Starting...");
  ESP_LOGI(TAG,"ESP32ArduinoFirebeetle");
  ESP_LOGI(TAG,"**** ESP32 Arduno Firebeetle board testing. ****");
  
  esp_info();
  initLed(LED);
  initPir(PIRPIN);
  if(nvs.begin("esp32", false))
    ESP_LOGI(TAG,"NVS Initialized.");
  else
    ESP_LOGE(TAG,"Error initializing NVS.");
  ESP_LOGI(TAG,"%u free entries in NVS.",nvs.freeEntries());
  nvs.end(); 

  semWifi = xSemaphoreCreateBinary();
  xSemaphoreGive(semWifi);
  xSemaphoreTake(semWifi, portMAX_DELAY);

  xTaskCreatePinnedToCore(&doInitWifiSta, "InitWifi", 8192, NULL, 0, &tskInitWifi, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  //xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, (void *) &ledBlinks, 0, &tskLedFlash, app_cpu);
  xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, NULL, 0, &tskLedFlash, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doNtp, "NTP", 2048, NULL, 0, &tskNtp, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doMqtt, "Mqtt", 2048, NULL, 0, &tskMqtt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doPir, "PIR", 2048, NULL, 0, &tskPir, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doOta, "OTA", 2048, NULL, 0, &tskOta, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
}

void loop() {
  // put your main code here, to run repeatedly:
  //LedBlink(2);
  vTaskDelay(2000/portTICK_PERIOD_MS);
}
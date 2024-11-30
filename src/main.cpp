/*
Code by Jeffrey Lehman

ESP32 DFRobot Firebeetle board testing

See README.md for more information

*/

#include "main.h"
static const char TAG[] = __FILE__;
int ledBlinks = 2;
TaskHandle_t tskLedFlash, tskPir, tskInitWifi, tskBleServer, tskReadBatt, tskReadTempInt, tskReadTempExt;
TaskHandle_t tskMqtt;
SemaphoreHandle_t semWifi;
Preferences nvs;
unsigned long tempIntAdc, tempIntOhms, tempExtAdc, tempExtOhms, BattAdc;
float tempIntDegF, tempExtDegF, BattVgpio, BattVoltage;


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

void doReadBatt(void * param) {
  unsigned long AdcVal;
  float Vbatt, Vgpio;
  
  ESP_LOGI(TAG,"ADC Batt TASK STARTING");
  analogReadResolution(12);
  // analogSetAttenuation(ADC_6db); 
  // analogSetPinAttenuation(BATTVPIN, ADC_6db); // 150mv - 1750 mv
  analogSetPinAttenuation(BATTVPIN, ADC_11db); // 150mv - 2450 mv
  // 150mv = 0, 2450mv = 4095
  adcAttachPin(BATTVPIN);


  while(true) {
    AdcVal = 0;
    for(int i=0; i<15; i++) {
      AdcVal += analogRead(BATTVPIN);
      vTaskDelay(50/portTICK_PERIOD_MS);
    }
    AdcVal /= 15;
    Vgpio = (float)AdcVal/4095 * 3.58;
    Vbatt = Vgpio * (99000 + 99700)/99700;
    ESP_LOGI(TAG,"ADC: %i, Vgpio %f, Vbatt %f",AdcVal, Vgpio, Vbatt);
    BattAdc = AdcVal;
    BattVgpio = Vgpio;
    BattVoltage = Vbatt;
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }

  ESP_LOGE(TAG,"Deleting ADC Batt Task");
  vTaskDelete(NULL);

}

void doReadTempInt(void * param) {
  unsigned long AdcVal, Rload;
  float Vgpio, tempF;
  
  ESP_LOGI(TAG,"ADC Temp Int TASK STARTING");
  analogReadResolution(12);
  analogSetPinAttenuation(TEMPINTPIN, ADC_6db); // 150mv - 1750 mv
  // analogSetPinAttenuation(BATTVPIN, ADC_11db); // 150mv - 2450 mv
  // 150mv = 0, 2450mv = 4095
  adcAttachPin(TEMPINTPIN);


  while(true) {
    AdcVal = 0;
    for(int i=0; i<15; i++) {
      AdcVal += analogRead(TEMPINTPIN);
      vTaskDelay(50/portTICK_PERIOD_MS);
    }
    AdcVal /= 15;
    // (AdcVal == 0)?AdcVal=1:AdcVal;      // Min Adc 1
    // (AdcVal >= 3880)?AdcVal=3879:AdcVal; // Max Adc 3955
    Vgpio = (float)AdcVal/4095 * 1.750 + 0.150;
    // Rload = 18710 * (Vgpio/3.3 - 1);
    Rload = Vgpio * 18710/(3.3 - Vgpio);
    tempF = (1/((1/298.15) + 1/3950.0*log((float)Rload/10000.0))-273.15)*9/5+32;
    ESP_LOGI(TAG,"ADC value: %i, resistance: %i, temp deg f: %f",AdcVal, Rload, tempF);
    tempIntAdc = AdcVal;
    tempIntOhms = Rload;
    tempIntDegF = tempF;
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }

  ESP_LOGE(TAG,"Deleting ADC Temp Int Task");
  vTaskDelete(NULL);

}

void doReadTempExt(void * param) {
  unsigned long AdcVal, Rload;
  float Vgpio, tempF;
  
  ESP_LOGI(TAG,"ADC Temp Ext TASK STARTING");
  analogReadResolution(12);
  analogSetPinAttenuation(TEMPEXTPIN, ADC_6db); // 150mv - 1750 mv
  // analogSetPinAttenuation(BATTVPIN, ADC_11db); // 150mv - 2450 mv
  // 150mv = 0, 2450mv = 4095
  adcAttachPin(TEMPEXTPIN);


  while(true) {
    AdcVal = 0;
    for(int i=0; i<15; i++) {
      AdcVal += analogRead(TEMPEXTPIN);
      vTaskDelay(50/portTICK_PERIOD_MS);
    }
    AdcVal /= 15;
    // (AdcVal == 0)?AdcVal=1:AdcVal;      // Min Adc 1
    // (AdcVal >= 3880)?AdcVal=3879:AdcVal; // Max Adc 3955
    Vgpio = (float)AdcVal/4095 * 1.750 + 0.150;
    // Rload = 21440 * (Vgpio/3.3 - 1);
    Rload = Vgpio * 21440/(3.3 - Vgpio);
    tempF = (1/((1/298.15) + 1/3950.0*log((float)Rload/10000.0))-273.15)*9/5+32;
    ESP_LOGI(TAG,"ADC value: %i, resistance: %i, temp deg f: %f",AdcVal, Rload, tempF);
    tempExtAdc = AdcVal;
    tempExtOhms = Rload;
    tempExtDegF = tempF;
    vTaskDelay(2000/portTICK_PERIOD_MS);
  }

  ESP_LOGE(TAG,"Deleting ADC Temp Ext Task");
  vTaskDelete(NULL);

}

void doLedFlash(void * param) {
  int numFlashes = 0;

  ESP_LOGI(TAG,"LED TASK STARTING");

  initLed(LED);

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
  ESP_LOGE(TAG,"Deleting LED TASK");
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
  ESP_LOGI(TAG,"ESP32 DFRobot Board");
  ESP_LOGI(TAG,"**** ESP32 Arduino DFRobot. ****");
  ESP_LOGI(TAG,"**** DFRobotESP32TempsBattProtoBoard Project ****");
  
  ESP_LOGE(TAG,"Log Error Message...");       // Lowest log level
  ESP_LOGW(TAG,"Log Warning Message...");
  ESP_LOGI(TAG,"Log Information Message...");
  ESP_LOGD(TAG,"Log Debug Message...");
  ESP_LOGV(TAG,"Log Verbose Message...");     // Highest log level

  resetReason = esp_reset_reason();
  ESP_LOGI(TAG,"Reset Reason: %i",resetReason);
  
  esp_info();

  if(nvs.begin("esp32", false))
    ESP_LOGI(TAG,"NVS Initialized.");
  else
    ESP_LOGE(TAG,"Error initializing NVS.");
  ESP_LOGI(TAG,"%u free entries in NVS.",nvs.freeEntries());
  nvs.end(); 

  #ifdef TEMP
  xTaskCreatePinnedToCore(&doReadBatt, "ReadBattV", 2048, NULL, 0, &tskReadBatt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doReadTempInt, "ReadTempInt", 2048, NULL, 0, &tskReadTempInt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doReadTempExt, "ReadTempExt", 2048, NULL, 0, &tskReadTempExt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  #endif

  #ifdef WIFI
  semWifi = xSemaphoreCreateBinary();
  xSemaphoreGive(semWifi);
  xSemaphoreTake(semWifi, portMAX_DELAY);

  // Re priority - higher number is higher priority
  xTaskCreatePinnedToCore(&doInitWifiSta, "InitWifi", 8192, NULL, 0, &tskInitWifi, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doNtp, "NTP", 2048, NULL, 0, &tskNtp, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doMqtt, "Mqtt", 8192, NULL, 0, &tskMqtt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  // xTaskCreatePinnedToCore(&doOta, "OTA", 2048, NULL, 0, &tskOta, app_cpu);
  // vTaskDelay(500/portTICK_PERIOD_MS);
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
  #ifdef SLEEP
  ESP_LOGI(TAG,"Sleeping.");
  // esp_sleep_enable_timer_wakeup(10000000); // 10 seconds
  esp_sleep_enable_timer_wakeup(1000000 * 60 * 10); // 10 minutes
  // esp_bluedroid_disable();
  // esp_bt_controller_disable();
  // esp_wifi_stop();
  esp_deep_sleep_start();
  #endif
}

void loop() {
  // put your main code here, to run repeatedly:
  //LedBlink(2);
  vTaskDelay(2000/portTICK_PERIOD_MS);
}
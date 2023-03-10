/*
ESP32 DFRobot Firebeetle board testing

Todo:
Serial1 output to another board
BLE server/client
BATTERY CHARging?
battery voltage?
battery level?
A/C power?

JSON format for MQTT messages

VBatt on A0 GPIO_NUM_36
Need to add voltage divider 1M and 1Meg


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
TaskHandle_t tskNtp, tskLedFlash, tskInitWifi, tskMqtt, tskPir, tskOta, tskBme280, tskBtnPress, tskTelnetSpy, tskNeoMatrix;
SemaphoreHandle_t semWifi;
Preferences nvs;
BlueDot_BME280 bme;
TelnetSpy ts;

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

// initialize Button on GPIO as input and Pull down
void initBtn(gpio_num_t pin) {
  gpio_config_t gpioBtn;
  gpioBtn.pin_bit_mask = ((1ULL<< pin));
  gpioBtn.mode         = GPIO_MODE_INPUT;
  gpioBtn.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpioBtn.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpioBtn.intr_type    = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&gpioBtn));
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

bool initI2c() {
  bool initResult;
  initResult = Wire.begin(SDAPIN, SCLPIN, 100000);
  Wire.setTimeOut(50);
  return initResult;
}

void doBme280(void * param) {

  bme.parameter.communication = 0; // I2C comm
  bme.parameter.I2CAddress = 0x77; // BME address 0x77
  bme.parameter.sensorMode = 0b11; // Sensor Mode
  bme.parameter.IIRfilter = 0b100;
  bme.parameter.humidOversampling = 0b101;
  bme.parameter.tempOversampling = 0b101;              //Temperature Oversampling for Sensor 1
  bme.parameter.pressOversampling = 0b101; 
  bme.parameter.pressureSeaLevel = 1013.25;  //default value of 1013.25 hPa (Sensor 
  bme.parameter.tempOutsideFahrenheit = 85; 

  while(bme.init() != 0x60) {
    SERIAL_PORT.printf("%lu: BME280 Sensor not found!\n", TS);
    bmeDetected = false;
    vTaskDelay(10000/portTICK_PERIOD_MS);
  }
  SERIAL_PORT.printf("%lu: BME280 Sensor found!\n", TS);
  bmeDetected = true;

  while(true) {
    // Read Wx info from BME280
    tempF = bme.readTempF() + TEMPOFFSET;
    humPct = bme.readHumidity() + HUMOFFSET;
    presshPa = bme.readPressure() + PRESSOFFSET;
    SERIAL_PORT.printf("%lu: Temp: %.1f F | RH: %.0f RH | Press: %.0f hPa\n", TS, tempF, humPct, presshPa);
    vTaskDelay(30000/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
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

void doBtnPress(void * param) {
  bool btnCurrentState;
  ESP_LOGI(TAG,"BUTTON TASK STARTING");

  btnCurrentState = gpio_get_level(BTNPIN);

  while(true) {
    if(gpio_get_level(BTNPIN) && btnCurrentState == 0) {
      SERIAL_PORT.printf("%lu: Button Pressed.\n", TS);
      btnCurrentState = 1;
      vTaskDelay(300/portTICK_PERIOD_MS);
    }
    else if(!gpio_get_level(BTNPIN) && btnCurrentState == 1) {
      SERIAL_PORT.printf("%lu: Button Released.\n", TS);
      btnCurrentState = 0;
      vTaskDelay(300/portTICK_PERIOD_MS);
    }
  }
  ESP_LOGI(TAG,"Deleting BUTTON TASK");
  vTaskDelete(NULL);
}

void doTelnetSpy(void* param) {
  while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
    ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.\n", 5000);
  }
  xSemaphoreGive(semWifi);
  SERIAL_PORT.begin(115200);
  ESP_LOGI(TAG,"TELNETSPY TASK STARTING");
  // SERIAL_PORT.setCallbackOnConnect(telnetConnected);
  // SERIAL_PORT.setCallbackOnDisconnect(telnetDisconnected);

  while(true) {
    ts.handle();
    
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
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
  initI2c();
  initBtn(BTNPIN);
  if(nvs.begin("esp32", false))
    ESP_LOGI(TAG,"NVS Initialized.");
  else
    ESP_LOGE(TAG,"Error initializing NVS.");
  ESP_LOGI(TAG,"%u free entries in NVS.",nvs.freeEntries());
  nvs.end(); 

  semWifi = xSemaphoreCreateBinary();
  xSemaphoreGive(semWifi);
  xSemaphoreTake(semWifi, portMAX_DELAY);

  // Re priority - higher number is higher priority
  xTaskCreatePinnedToCore(&doInitWifiSta, "InitWifi", 8192, NULL, 0, &tskInitWifi, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  //xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, (void *) &ledBlinks, 0, &tskLedFlash, app_cpu);
  xTaskCreatePinnedToCore(&doLedFlash, "LedFlash", 2048, NULL, 0, &tskLedFlash, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doTelnetSpy, "TelnetSpy", 8192, NULL, 0, &tskTelnetSpy, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doNtp, "NTP", 2048, NULL, 0, &tskNtp, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doMqtt, "Mqtt", 8192, NULL, 0, &tskMqtt, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doPir, "PIR", 2048, NULL, 0, &tskPir, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doOta, "OTA", 2048, NULL, 0, &tskOta, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doBme280, "BME280", 2048, NULL, 0, &tskBme280, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doBtnPress, "Button", 2048, NULL, 0, &tskBtnPress, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&doNeoLoop, "Neo Matrix", 2048, NULL, 0, &tskNeoMatrix, app_cpu);
  vTaskDelay(500/portTICK_PERIOD_MS);
}

void loop() {
  // put your main code here, to run repeatedly:
  //LedBlink(2);
  vTaskDelay(2000/portTICK_PERIOD_MS);
}
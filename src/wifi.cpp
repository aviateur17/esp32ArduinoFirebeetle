#include "jclwifi.h"

//extern bool wificonnected;
extern struct timeval tv;
time_t wifidisconnecttime = ULONG_MAX;
extern SemaphoreHandle_t semWifi;
extern TelnetSpy ts;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  SERIAL_PORT.printf("%lu: WiFi connected.\n", TS);
  SERIAL_PORT.printf("%lu: RSSI: %d\n", TS, WiFi.RSSI());
  delay(100);
  xSemaphoreGive(semWifi);
}

void doInitWifiSta(void * param) {
  int connectTries = 0;
  ESP_LOGI(TAG,"WIFI TASK STARTING");
  
  WiFi.config(IPAddress(HOSTIPADDRESS), IPAddress(GATEWAY), IPAddress(SUBNET), IPAddress(DNS1), IPAddress(DNS2));
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.begin(AP_SSID, AP_PASS);
  SERIAL_PORT.printf("%lu: WiFi MAC address: %s\n", TS, WiFi.macAddress().c_str());
  WiFi.setHostname(HOSTNAME);  
  
  SERIAL_PORT.printf("%lu: Attempting WiFi connection\n", TS);
  while(WiFi.status() != WL_CONNECTED && connectTries < 100) {
      SERIAL_PORT.print("w");
      vTaskDelay(1000/portTICK_PERIOD_MS);
      connectTries++;
      //WiFi.reconnect();
      // timeout??
  }
  SERIAL_PORT.println("");
  connectTries = 0;
  // if(WiFi.status() == WL_CONNECTED) {
  //   SERIAL_PORT.printf("%lu: WiFi connected\n", TS);
  //   xSemaphoreGive(semWifi);
  //   //wifidisconnecttime = ULONG_MAX;
  // }  
  
  while(true) {
    if(WiFi.status() != WL_CONNECTED) {
      SERIAL_PORT.printf("%lu: Attempting WiFi reconnection\n", TS);
      WiFi.disconnect();
      xSemaphoreTake(semWifi, portMAX_DELAY);
      WiFi.reconnect();
      vTaskDelay(10000/portTICK_PERIOD_MS);  
      // timeout??
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
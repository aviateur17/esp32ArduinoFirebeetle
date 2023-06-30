#include "jclwifi.h"

//extern bool wificonnected;
extern struct timeval tv;
time_t wifidisconnecttime = ULONG_MAX;
extern SemaphoreHandle_t semWifi;
// extern TelnetSpy ts;
static const char TAG[] = __FILE__;
uint8_t hostIpAddress[4], gatewayAddress[4], subnetAddress[4], dns2Address[4]; 
uint8_t dns1Address[4];

// Convert IP Addresses from strings to byte arrays of 4 bytes
void stringToByteArray(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGI(TAG, "%lu: WiFi connected.\n", TS);
  ESP_LOGI(TAG, "%lu: RSSI: %d\n", TS, WiFi.RSSI());
  delay(100);
  xSemaphoreGive(semWifi);
}

void doInitWifiSta(void * param) {
  int connectTries = 0;
  ESP_LOGI(TAG,"WIFI TASK STARTING");
  
  // Convert from String to byte array
  stringToByteArray(HOSTIPADDRESS, '.', hostIpAddress, 4, 10);
  stringToByteArray(GATEWAY, '.', gatewayAddress, 4, 10);
  stringToByteArray(SUBNET, '.', subnetAddress, 4, 10);
  stringToByteArray(DNS1, '.', dns1Address, 4, 10);
  stringToByteArray(DNS2, '.', dns2Address, 4, 10);
  WiFi.config(hostIpAddress, gatewayAddress, subnetAddress, dns1Address, dns2Address);
  // WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(AP_SSID, AP_PASS);
  ESP_LOGI(TAG, "%lu: WiFi MAC address: %s\n", TS, WiFi.macAddress().c_str());
  WiFi.setHostname(HOSTNAME);  
  
  ESP_LOGI(TAG, "%lu: Attempting WiFi connection\n", TS);
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
      ESP_LOGI(TAG, "%lu: Attempting WiFi reconnection\n", TS);
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
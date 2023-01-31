#include "jclmqtt.h"
#include <string>
#include <sys/time.h>

#define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)

WiFiClient espClient;
PubSubClient mqttclient(espClient);

extern struct timeval tv;
extern struct tm timeinfo;
extern  SemaphoreHandle_t semWifi;
//extern TelnetSpy ts;

void reconnect() {
  // Loop until we're reconnected
  while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
    ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.\n", 5000);
  }
  if(!mqttclient.connected()) {
    SERIAL_PORT.printf("%lu: Attempting MQTT connection...\n", TS);
    // Attempt to connect
    if(mqttclient.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS)) {
      SERIAL_PORT.printf("%lu: MQTT connected\n", TS);
      // Once connected, publish an announcement...
      //strncpy(mqtttopic, HOSTNAME, 100);
      //strncat(mqtttopic, "/heartbeatout", 100);
      //mqttclient.publish(mqtttopic, (&timeinfo != NULL) ? asctime(&timeinfo) : millis());
      //SERIAL_PORT.printf("%lu: Published %s\n", TS, mqtttopic);
      // ... and resubscribe
      //mqttclient.subscribe("heartbeatHUB/#");
    }
    else {
      SERIAL_PORT.printf("%lu: MQTT Connect failed, rc=%d\n", TS, mqttclient.state());
    }
  xSemaphoreGive(semWifi);
  }
}

void doMqtt(void * param) {
  ESP_LOGI(TAG,"MQTT TASK STARTING");
  // unsigned long lastMsg = millis();
  char mqtttopic[101];
  char mqttpayload[55];
  //uint iteration = 0;

  mqttclient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttclient.setKeepAlive(15);
  mqttclient.setSocketTimeout(3);
  
  while(xSemaphoreTake(semWifi,5000/portTICK_PERIOD_MS) == pdFALSE) {
    ESP_LOGD(TAG, "Failed to take Wifi Semaphore within %u ms.\n", 5000);
  }
  xSemaphoreGive(semWifi);

  
  while(true) {

    while(!mqttclient.connected()) {
      reconnect();
    }
    
    mqttclient.loop();

    if(mqttclient.connected()) {
      strncpy(mqtttopic, HOSTNAME, 100);
      strncat(mqtttopic, "/heartbeatout", 100);
      if(&timeinfo != NULL) {
      strncpy(mqttpayload, asctime(&timeinfo), 50);
      }
      else {
        snprintf(mqttpayload, 50, "%lu", millis());
      }
      mqttclient.publish(mqtttopic, mqttpayload);
      SERIAL_PORT.printf("%lu: Published %s\n", TS, mqtttopic);
      strncpy(mqtttopic, HOSTNAME, sizeof(HOSTNAME));
      mqttclient.publish(strncat(mqtttopic,"/appversion",27), APP_VERSION);
      //SERIAL_PORT.printf("%lu: Published %s\n", TS, mqtttopic);
      strncpy(mqtttopic, HOSTNAME, 100);
      strncat(mqtttopic, "/hostname", 100);
      mqttclient.publish(mqtttopic, HOSTNAME);
      strncpy(mqtttopic, HOSTNAME, 100);
      strncat(mqtttopic, "/ipaddress", 100);
      snprintf(mqttpayload, 25, "%s", WiFi.localIP().toString().c_str());
      mqttclient.publish(mqtttopic, mqttpayload);
    }
    mqttclient.disconnect();
    vTaskDelay(300000/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
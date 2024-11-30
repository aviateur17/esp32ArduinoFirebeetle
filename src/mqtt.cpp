#include "jclmqtt.h"

//#define TS ((tv.tv_sec == NULL) ? millis(): tv.tv_sec)

WiFiClient espClient;
PubSubClient mqttclient(espClient);

extern  SemaphoreHandle_t semWifi;
extern unsigned long tempIntAdc, tempIntOhms, tempExtAdc, tempExtOhms, BattAdc;
extern float tempIntDegF, tempExtDegF, BattVgpio, BattVoltage;
static const char TAG[] = __FILE__;

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
  char mqttpayload[2048];
  //uint iteration = 0;
  DynamicJsonDocument doc(2048);

  mqttclient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttclient.setKeepAlive(15);
  mqttclient.setSocketTimeout(3);
  mqttclient.setBufferSize(2048);
  
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
      doc["hostname"] = HOSTNAME;
      // if(&timeinfo != NULL) {
      //   doc["heartbeatout"] = asctime(&timeinfo);
      // }
      // else {
        doc["heartbeatout"] = millis();
      // }
      doc["program"] = PROGNAME;
      doc["appversion"] = APP_VERSION;
      // doc["temperature"] = tempF + TEMPOFFSET;
      // doc["relativehumidty"] = humPct + HUMOFFSET;
      // doc["pressure"] =  presshPa + PRESSOFFSET;
      doc["runtime"] = float(millis())/(1000.0*60*60*24);
      doc["time"] = doc["heartbeatout"];
      doc["ipaddress"] = WiFi.localIP().toString().c_str();
      doc["tempIntAdc"] = tempIntAdc;
      doc["tempIntOhms"] = tempIntOhms;
      doc["tempIntDegF"] = tempIntDegF;
      doc["tempExtAdc"] = tempExtAdc;
      doc["tempExtOhms"] = tempExtOhms;
      doc["tempExtDegF"] = tempExtDegF;
      doc["BattAdc"] = BattAdc;
      doc["BattVgpio"] = BattVgpio;
      doc["BattVoltage"] = BattVoltage;

      serializeJson(doc, mqttpayload, sizeof(mqttpayload));
      mqttclient.publish(MQTT_TOPIC, mqttpayload);


      // strncpy(mqtttopic, HOSTNAME, 100);
      // strncat(mqtttopic, "/heartbeatout", 100);
      // if(&timeinfo != NULL) {
      // strncpy(mqttpayload, asctime(&timeinfo), 50);
      // }
      // else {
      //   snprintf(mqttpayload, 50, "%lu", millis());
      // }
      // mqttclient.publish(mqtttopic, mqttpayload);
      // SERIAL_PORT.printf("%lu: Published %s\n", TS, mqtttopic);
      // strncpy(mqtttopic, HOSTNAME, sizeof(HOSTNAME));
      // mqttclient.publish(strncat(mqtttopic,"/appversion",27), APP_VERSION);
      // //SERIAL_PORT.printf("%lu: Published %s\n", TS, mqtttopic);
      // strncpy(mqtttopic, "host/", 100);
      // strncat(mqtttopic, HOSTNAME, 100);
      // strncat(mqtttopic, "/hostname", 100);
      // mqttclient.publish(mqtttopic, HOSTNAME);
      // if(bmeDetected) {
      //   strncpy(mqtttopic, "host/", 100);
      //   strncat(mqtttopic, HOSTNAME, 100);
      //   strncat(mqtttopic, "/temperature", 100);
      //   snprintf(mqttpayload, 6, "%5.2f deg F", tempF + TEMPOFFSET);
      //   mqttclient.publish(mqtttopic, mqttpayload);
      //   strncpy(mqtttopic, "host/", 100);
      //   strncat(mqtttopic, HOSTNAME, 100);
      //   strncat(mqtttopic, "/relativehumidity", 100);
      //   snprintf(mqttpayload, 6, "%2.2f RH", humPct + HUMOFFSET);
      //   mqttclient.publish(mqtttopic, mqttpayload);
      //   strncpy(mqtttopic, "host/", 100);
      //   strncat(mqtttopic, HOSTNAME, 100);
      //   strncat(mqtttopic, "/pressure", 100);
      //   snprintf(mqttpayload, 6, "%4.2f hPa", presshPa + PRESSOFFSET);
      //   mqttclient.publish(mqtttopic, mqttpayload);
      // }
      // strncpy(mqtttopic, "host/", 100);
      // strncat(mqtttopic, HOSTNAME, 100);
      // strncat(mqtttopic, "/runtime", 100);
      // snprintf(mqttpayload, 15, "%.4f days.", float(millis())/(1000.0*60*60*24));
      // mqttclient.publish(mqtttopic, mqttpayload);
      // strncpy(mqtttopic, "host/", 100);
      // strncat(mqtttopic, HOSTNAME, 100);
      // strncat(mqtttopic, "/time", 100);
      // mqttclient.publish(mqtttopic, asctime(&timeinfo));
      // strncpy(mqtttopic, "host/", 100);
      // strncat(mqtttopic, HOSTNAME, 100);
      // strncat(mqtttopic, "/ipaddress", 100);
      // snprintf(mqttpayload, 25, "%s", WiFi.localIP().toString().c_str());
      // mqttclient.publish(mqtttopic, mqttpayload);
    }
    //mqttclient.disconnect();
    //vTaskDelay(300000/portTICK_PERIOD_MS);
    vTaskDelay(60000/portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
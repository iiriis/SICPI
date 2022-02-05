#include "DHTStable.h"
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHT11_PIN 27
#define mq2 14
#define soilm 26
#define bulb 13
#define pump 12

DHTStable DHT;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

TaskHandle_t task1;

const char *ssid = "Dropkick";
const char *pswd = "avijitdasxp";
char *mqttServer = "saas.theakiro.com";
int mqttPort = 1883;


double temp = 0.0, humid = 0.0, soilraw = 0.0, mqraw = 0.0;
long ls=0;

void setup() {
  Serial.begin(115200);

  connectToWiFi();
  setupMQTT();
  if (!mqttClient.connected())
  connectMQTT();
  mqttClient.subscribe("Room");
  
  xTaskCreatePinnedToCore(readSensors, "Task1", 10000, NULL, 1, &task1, 0);

String t="";
char buf[10];

  for (;;)
  {
    
    if(millis()-ls>1000)
    {
    t=String(humid) + String(temp)+ String(soilraw)+ String(mqraw);
    t.toCharArray(buf,20);
    mqttClient.publish("sicpi", buf);
    ls=millis();
    }

    if (!mqttClient.connected())
    connectMQTT();
    mqttClient.loop();
  }

}

void loop()
{

}

void connectToWiFi()
{
  Serial.print("Connecting to ");

  WiFi.begin(ssid, pswd);
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to WiFi.");
}

void setupMQTT()
{
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void connectMQTT() 
{
  Serial.println("Connecting to Akiro...");
  while (!mqttClient.connected()) 
  {
    String clientId = "sicpi";
    if (mqttClient.connect(clientId.c_str(), "qWdhxyJzxnZY:@sicpi", "sicpi123" ))
      Serial.println("Connected to Akiro.");
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Callback - ");
  Serial.print("Message:");
  for (int i = 0; i < length; i++) 
    Serial.print((char)payload[i]);

  Serial.println();
}

void printSensors()
{
    Serial.print(humid);
    Serial.print(" ");
    Serial.print(temp);
    Serial.print(" ");
    Serial.print(mqraw);
    Serial.print(" ");
    Serial.println(soilraw);
}

void readSensors(void *pv)
{
  for (;;)
  {
    if (!DHT.read11(DHT11_PIN))
    {
      temp = DHT.getTemperature();
      humid = DHT.getHumidity();
    }

    mqraw = analogRead(mq2);
    soilraw = analogRead(soilm);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

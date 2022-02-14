#include "DHTStable.h"
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <U8x8lib.h>

#define DHT11_PIN 27
#define mq2 32
#define soilm 33
#define bulb 13
#define pump 12

DHTStable DHT;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

TaskHandle_t task1;
TaskHandle_t task2;
TaskHandle_t task3;

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(22, 21, U8X8_PIN_NONE);

const char *ssid = "Dropkick";
const char *pswd = "avijitdasxp";
char *mqttServer = "saas.theakiro.com";
int mqttPort = 1883;

int state = 0;
double temp = 0.0, humid = 0.0, soilraw = 0.0, mqraw = 0.0, setTemp = 0.0, setHumid = 0.0, setMoist = 0.0;
String t = "", rcv = "";
boolean seamaphore1 = true, seamaphore2 = false, isConnectedWiFi = false, stats = false;
char buf[150];
long ls = 0, setEpoch = 10;



void setup() 
{

  initSICPI();

  xTaskCreatePinnedToCore(readSensors, "Task1", 10000, NULL, 1, &task1, 0);
  xTaskCreatePinnedToCore(handleMQTT, "Task2", 10000, NULL, 2, &task2, 1);
  xTaskCreatePinnedToCore(driveOLED, "Task3", 10000, NULL, 3, &task3, 1);
}

void loop()
{

}

void initSICPI()
{
  Serial.begin(115200);
  pinMode(bulb, OUTPUT);
  pinMode(pump, OUTPUT);

  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_profont29_2x3_f);
  u8x8.clear();

  u8x8.drawString(0, 2, "SICPI");
  delay(2000);
  u8x8.clear();
  u8x8.setFont(u8x8_font_7x14_1x2_f);
  connectToWiFi();
  delay(2000);
  setupMQTT();
  delay(2000);
  if (!mqttClient.connected())
    connectMQTT();
  mqttClient.subscribe("Room");
}


void connectToWiFi()
{
  Serial.print("Connecting to ");
  u8x8.drawString(0, 0, "Connecting WiFi");
  int i = 0;
  WiFi.begin(ssid, pswd);
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    u8x8.drawString(i++, 2, ".");
    delay(500);
  }
  u8x8.clear();
  u8x8.drawString(0, 0, "WiFi");
  u8x8.drawString(0, 2, "Connected");
  Serial.println("Connected to WiFi.");
  state = 1;
}

void setupMQTT()
{
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

}

void connectMQTT()
{
  Serial.println("Connecting to Akiro...");
  u8x8.drawString(0, 0, "Connecting to ");
  u8x8.drawString(0, 2, "Akiro         ");

  while (!mqttClient.connected())
  {
    String clientId = "sicpi";
    if (mqttClient.connect(clientId.c_str(), "qWdhxyJzxnZY:@sicpi", "sicpi123" ))
      Serial.println("Connected to Akiro.");
  }
  delay(2000);
  u8x8.drawString(0, 0, "Connected to ");
  u8x8.drawString(0, 2, "Akiro       ");
}

void callback(char* topic, byte* payload, unsigned int length)
{
  rcv = "";

  for (int i = 0; i < length; i++)
    rcv = rcv + (char)payload[i];

  rcv.toLowerCase();

  handleAct();


}

void handleMQTT(void *pv2)
{
  for (;;)
  {
    if ((millis() - ls > setEpoch * 1000) && seamaphore1)
    {
      if (setTemp > 0 && setMoist > 0)
      {
        t = "Temperature : " + String(temp) + "\tHumidity : " + String(humid) + "\tMoisture : " + String(soilraw) + "\tC02 content : " + String(mqraw) + ", Stats: Active";
        seamaphore2 = true;
      }


      else
      {
        if (setTemp == 0.0 && setMoist == 0.0)
          t = "Temperature : " + String(temp) + "\tHumidity : " + String(humid) + "\tMoisture : " + String(soilraw) + "\tC02 content : " + String(mqraw) + "\tNote: Temperature and Moisture Not Set, Stats: Inactive";
        else if (setTemp == 0.0)
          t = "Temperature : " + String(temp) + "\tHumidity : " + String(humid) + "\tMoisture : " + String(soilraw) + "\tC02 content : " + String(mqraw) + "\tNote: Temperature Not Set, Stats: Inactive";
        else if (setMoist == 0.0)
          t = "Temperature : " + String(temp) + "\tHumidity : " + String(humid) + "\tMoisture : " + String(soilraw) + "\tC02 content : " + String(mqraw) + "\tNote: Moisture Not Set, Stats: Inactive";
      }

      t.toCharArray(buf, t.length() + 1);
      mqttClient.publish("sicpi", buf);
      ls = millis();
    }
    if (!mqttClient.connected())
      connectMQTT();
    mqttClient.loop();
  }
}


void handleAct()
{
  rcv.toLowerCase();

  if (rcv.startsWith("set"))
  {
    if (rcv.charAt(4) == 't')
    {
      t = rcv.substring(rcv.indexOf('=') + 2);
      setTemp = t.toFloat();
      t = "Setting Temperature to : " + t;
      t.toCharArray(buf, 35);
      mqttClient.publish("sicpi", buf);
    }

    else if (rcv.charAt(4) == 'm')
    {
      t = rcv.substring(rcv.indexOf('=') + 2);
      setMoist = t.toFloat();
      t = "Setting Moisture to : " + t;
      t.toCharArray(buf, 35);
      mqttClient.publish("sicpi", buf);
    }

    else if (rcv.charAt(4) == 'e')
    {
      t = rcv.substring(rcv.indexOf('=') + 2);
      setEpoch = t.toFloat();
      t = "Setting Epoch to : " + t;
      t.toCharArray(buf, 35);
      mqttClient.publish("sicpi", buf);
    }

    else
    {
      t = "Wrong Syntax";
      t.toCharArray(buf, 35);
      mqttClient.publish("sicpi", buf);
    }
  }

  else if (rcv.startsWith("stop"))
  {
    t = "Publishing Stopped";
    t.toCharArray(buf, 35);
    mqttClient.publish("sicpi", buf);
    seamaphore1 = false;
  }

  else if (rcv.startsWith("start"))
  {
    seamaphore1 = true;
    t = "Publishing Started";
    t.toCharArray(buf, 35);
    mqttClient.publish("sicpi", buf);
  }

  else
  {
    t = "Wrong Syntax";
    t.toCharArray(buf, 35);
    mqttClient.publish("sicpi", buf);
  }


  Serial.println(rcv);
  Serial.print(setHumid);
  Serial.print(" ");
  Serial.print(setTemp);
  Serial.print(" ");
  Serial.println(setMoist);

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
    printSensors();

    driveRelay();
    delay(500);

  }
}

void driveRelay()
{
  if (((setTemp - temp) > 2) && seamaphore2)
    digitalWrite(bulb, HIGH);
  else if (((temp - setTemp) > 2) && seamaphore2)
    digitalWrite(bulb, LOW);

  if (((soilraw - setMoist) > 100) && seamaphore2)
    digitalWrite(pump, HIGH);
  else if (((setMoist - soilraw) > 100) && seamaphore2)
    digitalWrite(pump, LOW);
}

void driveOLED(void *pv)
{
  String z1 = "", z2 = "", z3 = "", z4 = "";
  char p1[20], p2[20], p3[20], p4[20];
  for (;;)
  {
    delay(2000);
    
    if (!seamaphore2)
    {
      u8x8.clear();
      z1 = "Note temperature";
      z2 = "And/Or Humidity";
      z3 = "Not Set";
      z4 = "Stats: Inactive";
    }
    else
    {
      z1 = "Temp: " + String(temp);
      z2 = "Moist: " + String(soilraw);
      z3 = "AQ Index: " + String(mqraw);
      z4 = "Humidity: " + String(humid);
    }
    z1.toCharArray(p1, z1.length() + 1);
    z2.toCharArray(p2, z2.length() + 1);
    z3.toCharArray(p3, z3.length() + 1);
    z4.toCharArray(p4, z4.length() + 1);

    u8x8.drawString(0, 0, p1);
    u8x8.drawString(0, 2, p2);
    u8x8.drawString(0, 4, p3);
    u8x8.drawString(0, 6, p4);

  }
}

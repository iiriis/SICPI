#include "DHTStable.h"
#include <Wire.h>

DHTStable DHT;

#define DHT11_PIN 27
#define mq2 14
#define soilm 26

#define bulb 13
#define pump 12


double temp = 0.0, humid = 0.0, soilraw = 0.0, mqraw = 0.0;


void setup() {
  for (;;)
  { 

    readSensors();
    Serial.print(humid);
    Serial.print(" ");
    Serial.print(temp);
    Serial.print(" ");
    Serial.print(mqraw);
    Serial.print(" ");
    Serial.println(soilraw);  

  }  

}

void loop() {

}


void readSensors()
{
  if (!DHT.read11(DHT11_PIN))
  {
    temp = DHT.getTemperature();
    humid = DHT.getHumidity();
  }

  mqraw=analogRead(mq2);
  soilraw=analogRead(soilm); 
}

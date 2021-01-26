#include "DHT.h"
#include <SPI.h>  
#include "RF24.h"

#define DHTPIN 4  
#define DHTTYPE DHT22 

RF24 myRadio (7, 8);
byte addresses[][6] = {"0"};
const int led_pin = 13;

struct package
{
  float temperature ;
  float humidity ;
};


typedef struct package Package;
Package data;

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
    Serial.begin(9600);
    pinMode(led_pin, OUTPUT);
    dht.begin();
    myRadio.begin();  
    myRadio.setChannel(115); 
    myRadio.setPALevel(RF24_PA_MAX);
    myRadio.setDataRate( RF24_250KBPS ) ; 
    myRadio.openWritingPipe( addresses[0]);
    delay(1000);
}



void loop()
{
  digitalWrite(led_pin, HIGH); // Flash a light to show transmitting
  readSensor();
  Serial.println(data.humidity);
  Serial.println(data.temperature);
  myRadio.write(&data, sizeof(data)); 
  digitalWrite(led_pin, LOW);
  delay(1000);
}

void readSensor()
{
 data.humidity = dht.readHumidity();
 data.temperature = dht.readTemperature();
}



/*
 * temperature : i2c lcd 1602 display
 * humidity : i2c lcd 1602 display
 * rain : led (low/high)
 * water level : 7-segment display
 */

#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <dht11.h>
#include <SevSeg.h>

/* dht11 temperature and humidity module */
#define DHT11PIN A0
dht11 DHT11;

/* water level detection module */
#define WLPIN A1

/* i2c lcd 1602 display */
LiquidCrystal_I2C lcd(0x3f, 16, 2);

/* 7-segment display */
SevSeg sevseg; 

/* led (low/high) */
#define LEDPIN 13

#define TEMPHUMDELAY 5000
#define WLRAINDELAY 10000
#define LCDDELAY 5000
#define SEGDELAY 10000
#define LEDDELAY 10000

void TempHumUpdate( void *pvParameters );
void WaterLevelRainUpdate( void *pvParameters );
void LCDPrint( void *pvParameters );
void SEGPrint( void *pvParameters );
void LEDBlink( void *pvParameters );

struct package
{
  float temp; /* celsius */
  float hum; /* percentage */
  int waterlevel; /* cm */
  int rain; /* 0 : false, 1 : true */
} data;

SemaphoreHandle_t mutex;

void setup()
{
  //Serial.begin(9600);

  /* i2c lcd 1602 display */
  lcd.init();
  lcd.backlight();
  lcd.clear();

  /* 7-segment display */
  byte numDigits = 1;
  byte digitPins[] = {};
  byte segmentPins[] = {6, 5, 2, 3, 4, 7, 8, 9};
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_CATHODE; 
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);

  /* led (high/low) */
  pinMode(LEDPIN, OUTPUT);

  mutex = xSemaphoreCreateMutex();

  /* update */
  xTaskCreate( TempHumUpdate, "TempHumUpdate", 128, NULL, 1, NULL );
  xTaskCreate( WaterLevelRainUpdate, "WaterLevelRainUpdate", 128, NULL, 1, NULL );

  /* output */
  //xTaskCreate( LCDPrint, "LCDPrint", 128, NULL, 1, NULL );
  xTaskCreate( SEGPrint, "SEGPrint", 128, NULL, 1, NULL );
  //xTaskCreate( LEDBlink, "LEDBlink", 128, NULL, 1, NULL );
  
}

void loop()
{
  
}

// ----------------
//      update
// ----------------

/* 
 * input channel #1
 * dht11 temperature and humidity module
 * [temperature, humidity]
 */
void TempHumUpdate( void *pvParameters )
{
  float temp;
  float hum;

  for (;;)
  {
    DHT11.read(DHT11PIN);
    temp = (float)DHT11.temperature;
    hum = (float)DHT11.humidity;
    //temp = random(0, 50);
    //hum = random(10, 90);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.temp = temp;
      data.hum = hum;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( TEMPHUMDELAY / portTICK_PERIOD_MS );
  }
}

/* 
 * input channel #2
 * water level detection module
 * [rain]
 */
void WaterLevelRainUpdate( void *pvParameters )
{
  int waterlevel, waterlevel_old = 0;
  int rain = 0;

  for (;;)
  {
    waterlevel = analogRead(WLPIN);
    waterlevel = random(1000);

    /* water level increase -> raining */
    if (waterlevel > waterlevel_old) rain = 1;
    else rain = 0;
    //rain = random(0, 2);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.rain = rain;
      data.waterlevel = waterlevel;
      xSemaphoreGive(mutex);
    }

    waterlevel_old = waterlevel;
    
    vTaskDelay( WLRAINDELAY / portTICK_PERIOD_MS );
  }
}

// ----------------
//      output
// ----------------

/* 
 * output channel #1
 * i2c lcd 1602 display
 * [temperature, humidity]
 */
void LCDPrint( void *pvParameters )
{ 
  int temp, hum;
  
  for (;;) 
  {
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      temp = data.temp;
      hum = data.hum;
      xSemaphoreGive(mutex);
    }
      
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("TEMP: ");
    lcd.print(temp);
    lcd.print("C");
    
    lcd.setCursor(0, 1);
    lcd.print("HUM: ");
    lcd.print(hum);
    lcd.print("%");
    
    vTaskDelay( LCDDELAY / portTICK_PERIOD_MS );
  }
}

/* 
 * output channel #2
 * 7-segment display
 * [water level]
 */
void SEGPrint( void *pvParameters )
{
  int waterlevel;

  for (;;) 
  {
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      waterlevel = data.waterlevel;
      xSemaphoreGive(mutex);
    }

    /* cm conversion */
    waterlevel = waterlevel / 100; 
    /* only one digit available: 9+cm value is displayed as 9cm */
    if (waterlevel >= 10) waterlevel = 9; 
    
    sevseg.setNumber(waterlevel);
    sevseg.refreshDisplay();
    
    vTaskDelay( SEGDELAY / portTICK_PERIOD_MS );
  }
}

/* 
 * output channel #3
 * led (low/high)
 * [rain]
 */
void LEDBlink( void *pvParameters )
{ 
  int rain;
  
  for (;;) 
  {
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      rain = data.rain;
      xSemaphoreGive(mutex);
    }

    if (rain == 1) digitalWrite(LEDPIN, HIGH);
    else digitalWrite(LEDPIN, LOW);
    
    vTaskDelay( LEDDELAY / portTICK_PERIOD_MS );
  }
}

/*
   -------------------------
        weather station
   -------------------------

   temperature : dht11 module -> i2c lcd 1602 display
   humidity : dht11 module -> i2c lcd 1602 display
   water level : water detection module -> 7-segment display
   rain : water detection module -> led
   all : button -> serial monitor
*/

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <SevSeg.h>

/* dht11 temperature and humidity module */
#define DHTPIN 12
dht11 DHT11;

/* water level detection module */
#define WLPIN A1

/* i2c lcd 1602 display */
LiquidCrystal_I2C lcd(0x3f, 16, 2);

/* 7-segment display */
SevSeg sevseg;

/* led */
#define LEDPIN 13

/* button */
#define BUTTONPIN 11
int buttonState;

#define DHTDELAY 5000 /* temperature and humidity values update period */
#define WLDELAY 10000 /* water level and rain values update period*/
#define LCDDELAY 5000     /* i2c lcd 1602 display print period */
#define SEGDELAY 10000    /* 7-segment display print period */
#define LEDDELAY 10000    /* led blink period */
#define INITDELAY 2000    /* initial delay of the output channels */

void TempHumUpdate( void *pvParameters );
void WaterLevelRainUpdate( void *pvParameters );
void LCDPrint( void *pvParameters );
void SEGPrint( void *pvParameters );
void LEDBlink( void *pvParameters );

struct package
{
  float temp;     /* celsius */
  float hum;      /* percentage */
  int waterlevel; /* integer to be converted into millimeters */
  boolean rain;   /* boolean */
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

  /* led */
  pinMode(LEDPIN, OUTPUT);

  /* button */
  pinMode(BUTTONPIN, OUTPUT);
  buttonState = 0;

  mutex = xSemaphoreCreateMutex();

  /* update tasks */
  xTaskCreate( TempHumUpdate, "TempHumUpdate", 64, NULL, 2, NULL );
  xTaskCreate( WaterLevelRainUpdate, "WaterLevelRainUpdate", 64, NULL, 2, NULL );

  /* output tasks */
  xTaskCreate( LCDPrint, "LCDPrint", 128, NULL, 1, NULL );
  xTaskCreate( SEGPrint, "SEGPrint", 64, NULL, 1, NULL );
  xTaskCreate( LEDBlink, "LEDBlink", 64, NULL, 1, NULL );
  
}

void loop()
{

}

// --------------------------
//      update functions
// --------------------------

/*
   input channel #1
   dht11 temperature and humidity module
   [temperature, humidity]
*/
void TempHumUpdate( void *pvParameters )
{
  float temp;
  float hum;

  for (;;)
  {
    DHT11.read(DHTPIN);
    temp = (float)DHT11.temperature;
    hum = (float)DHT11.humidity;
    temp = random(0, 50);
    hum = random(10, 90);

    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.temp = temp;
      data.hum = hum;
      xSemaphoreGive(mutex);
    }

    vTaskDelay( DHTDELAY / portTICK_PERIOD_MS );
  }
}

/*
   input channel #2
   water level detection module
   [rain]
*/
void WaterLevelRainUpdate( void *pvParameters )
{
  int waterlevel, waterlevel_old = 0;
  boolean rain;

  for (;;)
  {
    waterlevel = analogRead(WLPIN);
    waterlevel = random(1000);

    /* increase in water level -> raining */
    if (waterlevel > 480 && waterlevel > waterlevel_old + 5) rain = true;
    else rain = false;
    //rain = random(0, 2);

    waterlevel_old = waterlevel;

    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.rain = rain;
      data.waterlevel = waterlevel;
      xSemaphoreGive(mutex);
    }

    vTaskDelay( WLDELAY / portTICK_PERIOD_MS );
  }
}

// --------------------------
//      output functions
// --------------------------

/*
   output channel #1
   i2c lcd 1602 display
   [temperature, humidity]
*/
void LCDPrint( void *pvParameters )
{
  /* initial delay to allow the sensor to collect initial data before displaying them */
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

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
   output channel #2
   7-segment display
   [water level]
*/
void SEGPrint( void *pvParameters )
{
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

  int waterlevel;
  int range;

  for (;;)
  {
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      waterlevel = data.waterlevel;
      xSemaphoreGive(mutex);
    }

    if (waterlevel <= 480) range = 0;                           /* 0mm */
    else if (waterlevel > 480 && waterlevel <= 530) range = 1;  /* 0mm+ to 5mm */
    else if (waterlevel > 530 && waterlevel <= 615) range = 2;  /* 5mm+ to 10mm */
    else if (waterlevel > 615 && waterlevel <= 660) range = 3;  /* 10mm+ to 15mm */
    else if (waterlevel > 660 && waterlevel <= 680) range = 4;  /* 15mm+ to 20mm */
    else if (waterlevel > 680 && waterlevel <= 690) range = 5;  /* 20mm+ to 25mm */
    else if (waterlevel > 690 && waterlevel <= 700) range = 6;  /* 25mm+ to 30mm */
    else if (waterlevel > 700 && waterlevel <= 705) range = 7;  /* 30mm+ to 35mm */
    else if (waterlevel > 705) range = 8;                       /* 35mm+ */

    sevseg.setNumber(range);
    sevseg.refreshDisplay();

    vTaskDelay( SEGDELAY / portTICK_PERIOD_MS );
  }
}

/*
   output channel #3
   led
   [rain]
*/
void LEDBlink( void *pvParameters )
{
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

  boolean rain;

  for (;;)
  {
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      rain = data.rain;
      xSemaphoreGive(mutex);
    }

    if (rain == true) digitalWrite(LEDPIN, HIGH);
    else digitalWrite(LEDPIN, LOW);

    vTaskDelay( LEDDELAY / portTICK_PERIOD_MS );
  }
}

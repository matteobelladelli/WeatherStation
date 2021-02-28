/*
 *  -------------------------
 *       weather station
 *  -------------------------
 *
 *  temperature : dht11 module -> i2c lcd 1602 display
 *  humidity : dht11 module -> i2c lcd 1602 display
 *  water level : water level detection module -> 7-segment display
 *  rain : water detection module -> led
 *  light : ldr -> i2c lcd 1602 display
 */

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <SevSeg.h>

/* dht11 module */
#define DHTPIN 12
dht11 DHT11;

/* water level detection module */
#define WLPIN A1
/* water level range boundaries, real value conversions in segprint function */
#define WL0 15
#define WL1 25
#define WL2 35
#define WL3 45
#define WL4 55
#define WL5 65
#define WL6 75
#define WL7 85
#define WL8 95

/* ldr */
#define LDRPIN A0 
#define LDRMAX 972
#define LDRMIN 340

/* i2c lcd 1602 display */
LiquidCrystal_I2C lcd(0x3f, 16, 2);

/* 7-segment display */
SevSeg sevseg;

/* led */
#define LEDPIN 13

/* task periods */
#define DHTDELAY 2000  /* temperature and humidity values update period */
#define WLDELAY 2000   /* water level and rain values update period*/
#define LDRDELAY 2000  /* light percentage value update period */
#define LCDDELAY 2000  /* i2c lcd 1602 display print period */
#define SEGDELAY 2000  /* 7-segment display print period */
#define LEDDELAY 20000 /* led blink period */
#define INITDELAY 2000 /* initial delay of the output channels */

/* task functions */
void DHTUpdate( void *pvParameters );
void WLUpdate( void *pvParameters );
void LDRUpdate( void *pvParameters );
void LCDPrint( void *pvParameters );
void SEGPrint( void *pvParameters );
void LEDBlink( void *pvParameters );

// -------------------------
//      data structures
// -------------------------

/*
 * temp : celsius
 * hum : percentage
 * waterlevel : integer to be converted into a millimeter range
 * light : integer to be converted into a percentage
 */
 
struct package_temphum {
  float temp;
  float hum;
} data_temphum;

struct package_wl {
  int waterlevel;
} data_wl;

struct package_light {
  int light;
} data_light;

SemaphoreHandle_t mutex_temphum;
SemaphoreHandle_t mutex_wl;
SemaphoreHandle_t mutex_light;

// ---------------
//      setup
// ---------------

void setup()
{
  /* dht11 module */
  pinMode(DHTPIN, INPUT);
  
  /* water level detection module */
  pinMode(WLPIN, INPUT);
    
  /* ldr */
  pinMode(LDRPIN, INPUT);

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

  /* mutexes */
  mutex_temphum = xSemaphoreCreateMutex();
  mutex_wl= xSemaphoreCreateMutex();
  mutex_light = xSemaphoreCreateMutex();

  /* update tasks */
  xTaskCreate( DHTUpdate, "DHTUpdate", 64, NULL, 2, NULL );
  xTaskCreate( WLUpdate, "WLUpdate", 64, NULL, 2, NULL );
  xTaskCreate( LDRUpdate, "LDRUpdate", 64, NULL, 2, NULL );

  /* output tasks */
  xTaskCreate( LCDPrint, "LCDPrint", 128, NULL, 1, NULL );
  xTaskCreate( SEGPrint, "SEGPrint", 64, NULL, 1, NULL );
  xTaskCreate( LEDBlink, "LEDBlink", 48, NULL, 1, NULL );
  
}

void loop() {}

// --------------------------
//      update functions
// --------------------------

/*
   input channel #1
   dht11 temperature and humidity module
   [temperature, humidity]
*/
void DHTUpdate( void *pvParameters )
{
  float temp;
  float hum;

  for (;;)
  {
    DHT11.read(DHTPIN);
    temp = (float)DHT11.temperature;
    hum = (float)DHT11.humidity;
    //temp = random(0, 50);
    //hum = random(10, 90);

    if (xSemaphoreTake(mutex_temphum, 5) == pdTRUE)
    {
      data_temphum.temp = temp;
      data_temphum.hum = hum;
      xSemaphoreGive(mutex_temphum);
    }

    vTaskDelay( DHTDELAY / portTICK_PERIOD_MS );
  }
}

/*
   input channel #2
   water level detection module
   [rain]
*/
void WLUpdate( void *pvParameters )
{
  int waterlevel;

  for (;;)
  {
    waterlevel = analogRead(WLPIN);
    //waterlevel = random(100);

    if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
    {
      data_wl.waterlevel = waterlevel;
      xSemaphoreGive(mutex_wl);
    }

    vTaskDelay( WLDELAY / portTICK_PERIOD_MS );
  }
}

/*
   input channel #3
   ldr
   [light]
*/
void LDRUpdate( void *pvParameters )
{
  int light;

  for (;;)
  {
    light = analogRead(LDRPIN);
    //light = random(0, 1024);

    if (xSemaphoreTake(mutex_light, 5) == pdTRUE)
    {
      data_light.light = light;
      xSemaphoreGive(mutex_light);
    }

    vTaskDelay( LDRDELAY / portTICK_PERIOD_MS );
  }
}

// --------------------------
//      output functions
// --------------------------

/*
   output channel #1
   i2c lcd 1602 display
   [temperature, humidity, light]
*/
void LCDPrint( void *pvParameters )
{
  /* initial delay to allow the sensors to collect initial data */
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

  int iteration = 0;

  float temp;
  float hum;
  int light;
  float lightpercentage;

  for (;;)
  {
    /* temperature and humidity */
    if (iteration == 0)
    {
      if (xSemaphoreTake(mutex_temphum, 5) == pdTRUE)
      {
        temp = data_temphum.temp;
        hum = data_temphum.hum;
        xSemaphoreGive(mutex_temphum);
      }
  
      lcd.clear();
  
      lcd.setCursor(0, 0);
      lcd.print("TEMP: ");
      lcd.print(temp);
      lcd.print((char)223);
      lcd.print("C");
  
      lcd.setCursor(0, 1);
      lcd.print("HUM: ");
      lcd.print(hum);
      lcd.print("%");
    }
    
    /* light */
    else
    {
      if (xSemaphoreTake(mutex_light, 5) == pdTRUE)
      {
        light = data_light.light;
        xSemaphoreGive(mutex_light);
      }
      
      /* percentage conversion */
      lightpercentage = (((float)(light - LDRMIN) / (LDRMAX - LDRMIN)) * 100);
  
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("LIGHT: ");
      lcd.print(lightpercentage);
      lcd.print("%");
    }

    iteration++;
    if (iteration > 1 ) iteration = 0;

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
    if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
    {
      waterlevel = data_wl.waterlevel;
      xSemaphoreGive(mutex_wl);
    }

    /* range conversion */
    if (waterlevel <= WL0) range = 0;                           /* 0mm */
    else if (waterlevel > WL0 && waterlevel <= WL1) range = 1;  /* 0+mm to 5mm */
    else if (waterlevel > WL1 && waterlevel <= WL2) range = 2;  /* 5+mm to 10mm */
    else if (waterlevel > WL2 && waterlevel <= WL3) range = 3;  /* 10+mm to 15mm */
    else if (waterlevel > WL3 && waterlevel <= WL4) range = 4;  /* 15+mm to 20mm */
    else if (waterlevel > WL4 && waterlevel <= WL5) range = 5;  /* 20+mm to 25mm */
    else if (waterlevel > WL5 && waterlevel <= WL6) range = 6;  /* 25+mm to 30mm */
    else if (waterlevel > WL6 && waterlevel <= WL7) range = 7;  /* 30+mm to 35mm */
    else if (waterlevel > WL7 && waterlevel <= WL8) range = 8;  /* 35+mm to 40mm */
    else if (waterlevel > WL8) range = 9;                       /* 40+mm */

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

  int waterlevel;
  int waterlevel_old = WL8;
  boolean rain;

  for (;;)
  {
    if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
    {
      waterlevel = data_wl.waterlevel;
      xSemaphoreGive(mutex_wl);
    }

    /* increase in water level -> raining */
    if (waterlevel > WL0 && waterlevel > waterlevel_old) rain = true;
    else rain = false;
    waterlevel_old = waterlevel;

    if (rain == true) digitalWrite(LEDPIN, HIGH);
    else digitalWrite(LEDPIN, LOW);

    vTaskDelay( LEDDELAY / portTICK_PERIOD_MS );
  }
}

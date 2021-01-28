/*
 * temperature : lcd display
 * humidity : lcd display
 * rain : led
 * barometic pressure : ???
 * wind : ???
 */

#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <DHT.h>

#define LEDPIN 13
#define DHTPIN 2
#define DHTTYPE DHT11

#define TEMPDELAY 5000
#define HUMDELAY 5000
#define RAINDELAY 10000
#define LCDDELAY 5000
#define LEDDELAY 10000

LiquidCrystal_I2C lcd(0x3f, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

void TempHumUpdate( void *pvParameters );
void RainUpdate( void *pvParameters );
void LCDPrint( void *pvParameters );
void LEDBlink( void *pvParameters );

struct package
{
  float temp;
  float hum;
  int rain; /* 0 : false, 1 : true */
} data;

SemaphoreHandle_t mutex;

void setup()
{
  //Serial.begin(9600);

  /* i2c lcd display */
  lcd.init();
  lcd.backlight();

  /* led */
  pinMode(LEDPIN, OUTPUT);

  /* dht11 sensor */
  dht.begin();

  mutex = xSemaphoreCreateMutex();

  /* update */
  xTaskCreate( TempHumUpdate, "TempHumUpdate", 128, NULL, 1, NULL );
  xTaskCreate( RainUpdate, "RainUpdate", 128, NULL, 1, NULL );

  /* output */
  xTaskCreate( LCDPrint, "LCDPrint", 128, NULL, 1, NULL );
  xTaskCreate( LEDBlink, "LEDBlink", 128, NULL, 1, NULL );
  
}

void loop()
{
  
}

// ----------------
//      update
// ----------------

void TempHumUpdate( void *pvParameters )
{
  float temp;
  float hum;

  for (;;)
  {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    //temp = random(0, 50);
    //hum = random(10, 90);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.temp = temp;
      data.hum = hum;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( TEMPDELAY / portTICK_PERIOD_MS );
  }
}

void RainUpdate( void *pvParameters )
{
  int rain;

  for (;;)
  {
    rain = random(0, 2);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.rain = rain;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( RAINDELAY / portTICK_PERIOD_MS );
  }
}

// ----------------
//      output
// ----------------

/* 
 * output channel #1: i2c lcd display 
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
 * output channel #2: led
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

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

#define LED_PIN 13

#define TEMP_DELAY 5000
#define HUM_DELAY 5000
#define RAIN_DELAY 5000
#define LCD_DELAY 5000
#define LED_DELAY 5000

LiquidCrystal_I2C lcd(0x3f, 16, 2);

void TempUpdate( void *pvParameters );
void HumUpdate( void *pvParameters );
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

  /* temperature and humidity i2c lcd display */
  lcd.init();
  lcd.backlight();

  /* rain blinking led */
  pinMode(LED_PIN, OUTPUT);

  mutex = xSemaphoreCreateMutex();

  /* update */
  xTaskCreate( TempUpdate, "TempUpdate", 128, NULL, 1, NULL );
  xTaskCreate( HumUpdate, "HumUpdate", 128, NULL, 1, NULL );
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

void TempUpdate( void *pvParameters )
{
  float temp;

  for (;;)
  {
    temp = random(0, 40);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.temp = temp;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( TEMP_DELAY / portTICK_PERIOD_MS );
  }
}

void HumUpdate( void *pvParameters )
{
  int hum;

  for (;;)
  {
    hum = random(10, 90);
    
    if (xSemaphoreTake(mutex, 5) == pdTRUE)
    {
      data.hum = hum;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( HUM_DELAY / portTICK_PERIOD_MS );
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
    
    vTaskDelay( RAIN_DELAY / portTICK_PERIOD_MS );
  }
}

// ----------------
//      output
// ----------------

/* 
 * output channel #1
 * temperature and humidity i2c lcd display
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
    
    vTaskDelay( LCD_DELAY / portTICK_PERIOD_MS );
  }
}

/* 
 * output channel #2
 * rain blinking led
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

    if (rain == 1) digitalWrite(LED_PIN, HIGH);
    else digitalWrite(LED_PIN, LOW);
    
    vTaskDelay( LED_DELAY / portTICK_PERIOD_MS );
  }
}

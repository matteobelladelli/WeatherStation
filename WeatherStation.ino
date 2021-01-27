/*
 * temperature : lcd display
 * humidity : lcd display
 * barometic pressure : ???
 * wind : ???
 * rain : ???
 */

#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#define LED_PIN 3

/* i2c lcd display */
LiquidCrystal_I2C lcd(0x3f, 16, 2);

void TempUpdate( void *pvParameters );
void HumUpdate( void *pvParameters );
void LCDPrint( void *pvParameters );

struct data
{
  float temp;
  float hum; 
} data;

SemaphoreHandle_t mutex;

void setup()
{
  Serial.begin(9600);

  /* i2c lcd display */
  lcd.init();
  lcd.backlight();
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 50);

  mutex = xSemaphoreCreateMutex();

  xTaskCreate( LCDPrint, "LCDPrint", 128, NULL, 1, NULL );
  xTaskCreate( TempUpdate, "TempUpdate", 128, NULL, 1, NULL );
  xTaskCreate( HumUpdate, "HumUpdate", 128, NULL, 1, NULL );
}

void loop()
{

}

void TempUpdate( void *pvParameters )
{
  int temp = 20.;

  for (;;)
  {
    temp = random(0, 40);
    
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      data.temp = temp;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

void HumUpdate( void *pvParameters )
{
  int hum = 70.;

  for (;;)
  {
    hum = random(10, 90);
    
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      data.hum = hum;
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

void LCDPrint( void *pvParameters )
{ 
  for (;;) 
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      lcd.clear();
  
      lcd.setCursor(0, 0);
      lcd.print("TEMP: ");
      lcd.print(data.temp);
      lcd.print("C");
      
      lcd.setCursor(0, 1);
      lcd.print("HUM: ");
      lcd.print(data.hum);
      lcd.print("%");
      
      xSemaphoreGive(mutex);
    }
    
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

/*
 *  -------------------------
 *       weather station
 *  -------------------------
 *
 *  temperature : dht11 module -> i2c lcd 1602 display
 *  humidity : dht11 module -> i2c lcd 1602 display
 *  water level : water detection module -> i2c lcd 1602 display
 *  rain : water detection module -> led
 *  light : ldr -> i2c lcd 1602 display
 */

// use arduino uno board
#define UNO
// use random generated values
//#define RAND

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <LiquidCrystal_I2C.h>
#include <dht11.h>

/* dht11 module */
#define DHTPIN 12
dht11 DHT11;

/* water level detection module */
#define WLPIN A1
/* water level range boundaries */
#define WL0 480 /* 0mm */
#define WL1 530 /* 0+mm to 5mm */
#define WL2 615 /* 5+mm to 10mm */
#define WL3 660 /* 10+mm to 15mm */
#define WL4 680 /* 15+mm to 20mm */
#define WL5 690 /* 20+mm to 25mm */
#define WL6 700 /* 30+mm to 35mm */
#define WL7 705 /* 35+mm to 40mm */
#define WL8 710 /* 40+mm */

/* ldr */
#define LDRPIN A0 
#define LDRMAX 1024
#define LDRMIN 0

/* i2c lcd 1602 display */
LiquidCrystal_I2C lcd(0x3f, 16, 2);

/* led */
#define LEDPIN 13

/* button */
#define BTNPIN 2
#define BTNPRESSED 0
#define BTNNOTPRESSED 1

/* task functions */
void DHTUpdate( void *pvParameters );
void WLUpdate( void *pvParameters );
void LDRUpdate( void *pvParameters );
void BTNRead( void *pvParameters );
void LCDPrint( void *pvParameters );
void LEDBlink( void *pvParameters );
#ifndef UNO
void SerialPrint( void *pvParameters );
#endif

/* task periods */
#define DHTDELAY  2000   /* temperature and humidity values update period */
#define WLDELAY   2000   /* water level value update period*/
#define LDRDELAY  2000   /* light value update period */
#define LCDDELAY  2000   /* i2c lcd 1602 display print period */
#define LEDDELAY  20000  /* led blink period */
#define SERIALDELAY 2000 /* serial refresh period */
#define INITDELAY 200    /* initial delay of the output channels to allow the sensors to collect initial data */
#define BTNDELAY  50     /* button read period */

// -------------------------
//      data structures
// -------------------------

/*
 * temp : celsius
 * hum : percentage
 * waterlevel : integer to be converted into a millimeter value
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
SemaphoreHandle_t interruptsem;

// ---------------
//      setup
// ---------------

void setup()
{
  /* serial monitor */
  #ifndef UNO
  Serial.begin(9600);
  while (!Serial);
  #endif
  
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

  /* led */
  pinMode(LEDPIN, OUTPUT);

  /* button */
  pinMode(BTNPIN, INPUT);
  
  /* semaphores */
  mutex_temphum = xSemaphoreCreateMutex();
  mutex_wl= xSemaphoreCreateMutex();
  mutex_light = xSemaphoreCreateMutex();
  interruptsem = xSemaphoreCreateBinary();

  /* update tasks */
  xTaskCreate( DHTUpdate, "DHTUpdate", 64, NULL, 2, NULL );
  xTaskCreate( WLUpdate, "WLUpdate", 64, NULL, 2, NULL );
  xTaskCreate( LDRUpdate, "LDRUpdate", 64, NULL, 2, NULL );

  /* interrupt tasks */
  xTaskCreate( BTNRead, "BTNRead", 64, NULL, 2, NULL );

  /* output tasks */
  xTaskCreate( LCDPrint, "LCDPrint", 144, NULL, 1, NULL );
  xTaskCreate( LEDBlink, "LEDBlink", 48, NULL, 1, NULL );
  #ifndef UNO
  xTaskCreate( SerialPrint, "SerialPrint", 82, NULL, 1, NULL );
  #endif

  vTaskStartScheduler();
  
}

void loop() {}

// --------------------------
//      update functions
// --------------------------

/*
 * input channel #1: dht11 temperature and humidity module
 * for: updating temperature and humidity
 */
void DHTUpdate( void *pvParameters )
{
  (void) pvParameters;
  
  float temp;
  float hum;

  for (;;)
  {
    DHT11.read(DHTPIN);
    temp = (float)DHT11.temperature;
    hum = (float)DHT11.humidity;
    #ifdef RAND
    temp = random(0., 50.);
    hum = random(10., 90.);
    #endif

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
 * input channel #2: water level detection module
 * for: updating water level
 */
void WLUpdate( void *pvParameters )
{
  (void) pvParameters;
  
  int waterlevel;

  for (;;)
  { 
    waterlevel = analogRead(WLPIN);
    #ifdef RAND
    waterlevel = random(400, 720);
    #endif

    if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
    {
      data_wl.waterlevel = waterlevel;
      xSemaphoreGive(mutex_wl);
    }

    vTaskDelay( WLDELAY / portTICK_PERIOD_MS );
  }
}

/*
 * input channel #3: ldr
 * for: updating light
 */
void LDRUpdate( void *pvParameters )
{
  (void) pvParameters;
  
  int light;

  for (;;)
  {
    light = analogRead(LDRPIN);
    #ifdef RAND
    light = random(0, 1024);
    #endif

    if (xSemaphoreTake(mutex_light, 5) == pdTRUE)
    {
      data_light.light = light;
      xSemaphoreGive(mutex_light);
    }

    vTaskDelay( LDRDELAY / portTICK_PERIOD_MS );
  }
}

// -----------------------------
//      interrupt functions
// -----------------------------

/*
 * input channel #4: button
 * for: changing page on i2c lcd 1602 display
 */

void BTNRead( void *pvParameters )
{
  (void) pvParameters;

  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );
  
  TickType_t last_wake_time;
  int curr_state, prev_state = BTNNOTPRESSED; // states for the button
  const TickType_t sample_interval = BTNDELAY / portTICK_PERIOD_MS;

  // initialise the last_wake_time variable with the current time
  last_wake_time = xTaskGetTickCount();

  for (;;)
  {
    // wait for the next cycle
    vTaskDelayUntil( &last_wake_time, sample_interval );
    // get the level on button pin
    curr_state = digitalRead(BTNPIN);
    if ((curr_state == BTNPRESSED) && (prev_state == BTNNOTPRESSED)){
        xSemaphoreGiveFromISR(interruptsem, NULL);
    }
    prev_state = curr_state;
  }
}

// --------------------------
//      output functions
// --------------------------

/* water level conversion to millimeter value */
float waterlevel_conversion(int waterlevel)
{
    float waterlevel_norm;
    
    if (waterlevel <= WL0) waterlevel_norm = 0;
    else if (waterlevel > WL0 && waterlevel <= WL1) waterlevel_norm = 0  + ((float)(waterlevel - WL0) / (WL1 - WL0)) * 5;
    else if (waterlevel > WL1 && waterlevel <= WL2) waterlevel_norm = 5  + ((float)(waterlevel - WL1) / (WL2 - WL1)) * 5;
    else if (waterlevel > WL2 && waterlevel <= WL3) waterlevel_norm = 10 + ((float)(waterlevel - WL2) / (WL3 - WL2)) * 5;
    else if (waterlevel > WL3 && waterlevel <= WL4) waterlevel_norm = 15 + ((float)(waterlevel - WL3) / (WL4 - WL3)) * 5;
    else if (waterlevel > WL4 && waterlevel <= WL5) waterlevel_norm = 20 + ((float)(waterlevel - WL4) / (WL5 - WL4)) * 5;
    else if (waterlevel > WL5 && waterlevel <= WL6) waterlevel_norm = 25 + ((float)(waterlevel - WL5) / (WL6 - WL5)) * 5;
    else if (waterlevel > WL6 && waterlevel <= WL7) waterlevel_norm = 30 + ((float)(waterlevel - WL6) / (WL7 - WL6)) * 5;
    else if (waterlevel > WL7 && waterlevel <= WL8) waterlevel_norm = 35 + ((float)(waterlevel - WL7) / (WL8 - WL7)) * 5;
    else if (waterlevel > WL8) waterlevel_norm = 40;

    return waterlevel_norm;
}

/* light conversion to percentage value */
float light_conversion(int light)
{
  float light_pct;
  
  light_pct = ((float)(light - LDRMIN) / (LDRMAX - LDRMIN)) * 100;
  if (light_pct < 0) light_pct = 0;
  if (light_pct > 100) light_pct = 100;
  
  return light_pct;
}

/*
 * output channel #1: i2c lcd 1602 display
 * for: displaying temperature, humidity, water level and light
 */
void LCDPrint( void *pvParameters )
{
  (void) pvParameters;
  
  /* initial delay to allow the sensors to collect initial data */
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

  // displayed page {0 : temperature & humidity, 1 : water level & light}
  int page = -1;

  float temp;
  float hum;
  int waterlevel;
  float waterlevel_norm;
  int light;
  float light_pct;

  for (;;)
  {
    // wait lcddelay or button interrupt for updating data
    if (xSemaphoreTake(interruptsem, LCDDELAY / portTICK_PERIOD_MS) == pdPASS) {
      page++;
      if (page > 1) page = 0;
    }

    /* temperature and humidity */
    if (page == 0)
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

    /* water level and light */
    else
    {
      if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
      {
        waterlevel = data_wl.waterlevel;
        xSemaphoreGive(mutex_wl);
      }
      
      if (xSemaphoreTake(mutex_light, 5) == pdTRUE)
      {
        light = data_light.light;
        xSemaphoreGive(mutex_light);
      }

      waterlevel_norm = waterlevel_conversion(waterlevel);
      light_pct = light_conversion(light);

      lcd.clear();
      
      lcd.setCursor(0, 0);
      lcd.print("WL: ");
      if (waterlevel <= WL8) lcd.print(waterlevel_norm);
      else lcd.print("40.00+");
      lcd.print("mm");
      
      lcd.setCursor(0, 1);
      lcd.print("LIGHT: ");
      lcd.print(light_pct);
      lcd.print("%");
    }

  }
}

/*
 * output channel #2: led
 * for: displaying rain
 */
void LEDBlink( void *pvParameters )
{
  (void) pvParameters;
    
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );

  int waterlevel;
  int waterlevel_old = 720;
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

/*
 * output channel #3: serial monitor
 * for: graphical representation using python matplotlib
 */
#ifndef UNO
void SerialPrint( void *pvParameters )
{
  (void) pvParameters;
    
  vTaskDelay( INITDELAY / portTICK_PERIOD_MS );
  
  float temp;
  float hum;
  int waterlevel;
  int waterlevel_norm;
  int light;
  int light_pct;

  for (;;)
  {        
    if (xSemaphoreTake(mutex_temphum, 5) == pdTRUE)
    {
      temp = data_temphum.temp;
      hum = data_temphum.hum;
      xSemaphoreGive(mutex_temphum);
    }
    if (temp < 0) temp = 0;
    if (temp > 50) temp = 50;

    if (xSemaphoreTake(mutex_wl, 5) == pdTRUE)
    {
      waterlevel = data_wl.waterlevel;
      xSemaphoreGive(mutex_wl);
    }
    
    if (xSemaphoreTake(mutex_light, 5) == pdTRUE)
    {
      light = data_light.light;
      xSemaphoreGive(mutex_light);
    }

    waterlevel_norm = (int)waterlevel_conversion(waterlevel);
    light_pct = (int)light_conversion(light);
    
    Serial.write((int)temp);
    Serial.write((int)hum);
    Serial.write(waterlevel_norm);
    Serial.write(light_pct);

    vTaskDelay( SERIALDELAY / portTICK_PERIOD_MS );
  }
}
#endif

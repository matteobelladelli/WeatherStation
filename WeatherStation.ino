#include <Arduino_FreeRTOS.h>
#include <semphr.h>

SemaphoreHandle_t xSerialSemaphore;

TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;
TaskHandle_t TaskHandle_3;

void TaskFunc_1(void *param);
void TaskFunc_2(void *param);
void TaskFunc_3(void *param);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  if (xSerialSemaphore == NULL)
  {
    xSerialSemaphore = xSemaphoreCreateMutex();
    if ((xSerialSemaphore) != NULL)
      xSemaphoreGive(xSerialSemaphore);
  }

  xTaskCreate(TaskFunc_1, "Task_1", 128, NULL, 1, &TaskHandle_1);
  xTaskCreate(TaskFunc_2, "Task_2", 128, NULL, 2, &TaskHandle_2);
  xTaskCreate(TaskFunc_3, "Task_3", 128, NULL, 3, &TaskHandle_3);

}

void loop() {
  // put your main code here, to run repeatedly:

}

void TaskFunc_1(void *param)
{
  (void) param;


  for (;;)
  {
    if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 5) == pdTRUE)
    {
      Serial.println("Task_1");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    
      xSemaphoreGive(xSerialSemaphore);
    }
  }
}

void TaskFunc_2(void *param)
{
  (void) param;

  for (;;)
  {
    if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 5) == pdTRUE)
    {
      Serial.println("Task_2");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    
      xSemaphoreGive(xSerialSemaphore);
    }
  }
}

void TaskFunc_3(void *param)
{
  (void) param;

  for (;;)
  {
    if (xSemaphoreTake(xSerialSemaphore, (TickType_t) 5) == pdTRUE)
    {
      Serial.println("Task_3");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    
      xSemaphoreGive(xSerialSemaphore);
    }
  }
}

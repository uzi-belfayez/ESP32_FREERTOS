#include <Wire.h>
#include "SSD1306Wire.h"
#include "images.h"
#include "DHTesp.h"
#include <Ticker.h>
#include <Arduino.h>


#define SDA GPIO_NUM_5
#define SCL GPIO_NUM_4
#define TAILLE_MAX 30


typedef struct {
  float mesure;
  char type_capteur;
} mesure_t;

int dhtPin = 17;

DHTesp dht;

void setup_dht() {
  Serial.begin(115200);
  dht.setup(17, DHTesp::DHT22); 
}

SSD1306Wire display(0x3c, SDA, SCL);

mesure_t tab_mesure[TAILLE_MAX] ; 
SemaphoreHandle_t s1 = NULL; 
SemaphoreHandle_t s2 = NULL; 
SemaphoreHandle_t mutex = NULL ; 

int table_pointer = 0 ; 

float temp,humidite ;


void vProducteurTemperature(void *pvParameters)
{
    const char *pcTaskName = "ProducteurTemperature";
    int valueToSend;
    BaseType_t status;
    UBaseType_t uxPriority;
    valueToSend = (int)pvParameters;
    uxPriority = uxTaskPriorityGet(NULL);
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    float temp;
    mesure_t mesure_container_temp;

    for (;;)
    {
        // Read Temperature
        TempAndHumidity data = dht.getTempAndHumidity();
        temp = data.temperature ;
        mesure_container_temp.mesure = temp;
        mesure_container_temp.type_capteur = 'T';

        // Publish to buffer
        xSemaphoreTake(s1, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        tab_mesure[table_pointer] = mesure_container_temp;
        table_pointer = (table_pointer + 1) % TAILLE_MAX;
        xSemaphoreGive(mutex);
        xSemaphoreGive(s2);
        printf("ProducteurTemperature produced %f \n", temp);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}

void vProducteurHumidite(void *pvParameters)
{
    const char *pcTaskName = "ProducteurHumidite";
    int valueToSend;
    BaseType_t status;
    UBaseType_t uxPriority;
    valueToSend = (int)pvParameters;
    uxPriority = uxTaskPriorityGet(NULL);
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    float humidite;
    mesure_t mesure_container_humidite;

    for (;;)
    {
        // Read humidity
        TempAndHumidity data = dht.getTempAndHumidity();
        humidite = data.humidity ;
        mesure_container_humidite.mesure = humidite;
        mesure_container_humidite.type_capteur = 'H';

        // Publish to buffer
        xSemaphoreTake(s1, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        tab_mesure[table_pointer] = mesure_container_humidite;
        table_pointer = (table_pointer + 1) % TAILLE_MAX;
        xSemaphoreGive(mutex);
        xSemaphoreGive(s2);
        printf("ProducteurHumidité produced %f \n", humidite);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}


void vConsomateur(void *pvParameters)
{
  const char *pcTaskName = "Consomateur";
  UBaseType_t uxPriority;
  uxPriority = uxTaskPriorityGet(NULL);
  int i = 0;
  mesure_t current_mesure ;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  for (;;)
  {   
      
      xSemaphoreTake(s2, portMAX_DELAY); 
      // Consume value
      current_mesure = tab_mesure[i] ; 
      i = (i + 1) % TAILLE_MAX;
      xSemaphoreGive(s1); 
      if (current_mesure.type_capteur == 'H') {
        humidite = current_mesure.mesure ; 
        printf("Le consomateur a consomé %f de type Humidité \n ",humidite);
      }
      else if (current_mesure.type_capteur == 'T'){
        temp = current_mesure.mesure ;
        printf("Le consomateur a consomé %f de type temp \n ",temp);

      }
      // else if (current_mesure.type_capteur == 'C'){
      //   taux_co2 = current_mesure.mesure ; 
      //   printf("Le consomateur a consomé %d de type C02 \n ",taux_co2);
      // }

      // if (interruptOccurred){
      // display.clear();
      // update_screen();
      // }

      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
  
  }
  vTaskDelete(NULL); 
}



void update_screen() {

  display.drawXbm(0, 0, image_width, image_height, sensor_screen_bits);
  // Convert temperature and humidity to strings
  char tempStr[8]; // Buffer to hold temperature string
  char humidityStr[8]; 
  //char co2Str[8] ;

  //sprintf(co2Str, "%d", taux_co2);

  char ppm[4] = "ppm"; 

  dtostrf(temp, 4, 1, tempStr); // Convert float to string with 1 decimal place
  dtostrf(humidite, 4, 1, humidityStr); // Convert float to string with 1 decimal place
  strcat(tempStr, "°C");
  strcat(humidityStr, "%");
  display.setFont(ArialMT_Plain_16);

  // Display temperature
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(42, 8, tempStr);

  // Display humidity
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(42, 40, humidityStr);

  // Display Co2
  // display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.drawString(100, 30, co2Str);
  // display.setFont(ArialMT_Plain_10);
  // display.drawString(100, 44, ppm);

  display.display();
}





void setup() {
  setup_dht();
  
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  s1 = xSemaphoreCreateCounting( TAILLE_MAX, TAILLE_MAX );
  s2 = xSemaphoreCreateCounting( TAILLE_MAX, 0 );
  mutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore( vProducteurTemperature, "ProducteurTemp", 10000, NULL, 1, NULL , 0 ); 
  xTaskCreatePinnedToCore( vProducteurHumidite, "ProducteurHumidite", 10000, NULL, 1, NULL , 0 );
  xTaskCreatePinnedToCore( vConsomateur, "Consomateur", 10000, NULL, 1 ,NULL ,  0 );

}

void loop() {
  vTaskDelay(portMAX_DELAY);  
} 
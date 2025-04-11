#include <Wire.h>
#include "SSD1306Wire.h"
#include "images.h"
#include "DHTesp.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Redmi Note 9 Pro";
const char* password = "3ezdinblid";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// JSON document for sensor data
StaticJsonDocument<200> jsonDoc;
#include <Ticker.h>
#include <Arduino.h>
#include <HardwareSerial.h>


#define SDA GPIO_NUM_5
#define SCL GPIO_NUM_4
#define TAILLE_MAX 30

#define TXESP_PIN 17 //1
#define RXESP_PIN 16 //3

HardwareSerial SerialSensor(2);



typedef struct {
  float mesure;
  char type_capteur;
} mesure_t;

int dhtPin = 18;

DHTesp dht;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  }
}

void setup_dht() {
  Serial.begin(115200);
  dht.setup(dhtPin, DHTesp::DHT22);
}

SSD1306Wire display(0x3c, SDA, SCL);

mesure_t tab_mesure[TAILLE_MAX] ; 
SemaphoreHandle_t s1 = NULL; 
SemaphoreHandle_t s2 = NULL; 
SemaphoreHandle_t mutex = NULL ; 

int table_pointer = 0 ; 

float temp, humidite, co2 ;

void update_screen_v1() {
  display.displayOn();
  display.clear(); // Clear previous display

  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  char tempStr[10];
  char humidityStr[10];
  char co2Str[10];

  // Format temperature and humidity
  dtostrf(temp, 4, 1, tempStr);
  dtostrf(humidite, 4, 1, humidityStr);
  dtostrf(co2, 4, 1, co2Str);
  strcat(tempStr, "°C");
  strcat(humidityStr, "%");
  strcat(co2Str, "ppm");

  display.drawString(64, 5, "Temp: " + String(tempStr));
  display.drawString(64, 25, "Hum: " + String(humidityStr));
  display.drawString(64, 40, "CO2: " + String(co2Str));

  display.display(); // Send buffer to screen
}

void shutdown_screen() {
  display.clear();            
  display.display();          
  display.displayOff();       
}


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
        printf("ProducteurTemperature produced %.2f \n", temp);
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
        printf("ProducteurHumidité produced %.2f \n", humidite);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
    }
}


String input = "";

unsigned long lastProximityTime = 0;
bool screenOn = false;
const unsigned long proximityTimeout = 3000;  // 3 seconds

void vUARTReceiver(void *pvParameters) {
  const char *pcTaskName = "ProducteurCO2";
  int valueToSend;
  BaseType_t status;
  UBaseType_t uxPriority;
  valueToSend = (int)pvParameters;
  uxPriority = uxTaskPriorityGet(NULL);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  mesure_t mesure_container_co2;
  float co2;

  for (;;) {
    // 1. Handle incoming messages
    if (SerialSensor.available()) {
      String input = SerialSensor.readStringUntil('\n');
      input.trim();
      Serial.println("Received: " + input);

      if (input == "Object detected at 50 cm!") {
        lastProximityTime = millis();
        if (!screenOn) {
          update_screen_v1();  // turn ON screen
          screenOn = true;
        }
      } 
      else if (input.length() > 0 && isDigit(input.charAt(0))) {
        co2 = input.toFloat();
        mesure_container_co2.mesure = co2;
        mesure_container_co2.type_capteur = 'C';

        xSemaphoreTake(s1, portMAX_DELAY);
        xSemaphoreTake(mutex, portMAX_DELAY);
        tab_mesure[table_pointer] = mesure_container_co2;
        table_pointer = (table_pointer + 1) % TAILLE_MAX;
        xSemaphoreGive(mutex);
        xSemaphoreGive(s2);
        printf("ProducteurCO2 produced %.2f \n", co2);
      }
    }

    // 2. Check timeout to shut down screen
    if (screenOn && (millis() - lastProximityTime > proximityTimeout)) {
      shutdown_screen();  // turn OFF screen
      screenOn = false;
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
  }

  vTaskDelete(NULL);
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
        printf("Le consomateur a consomé %.2f de type Humidité \n ",humidite);
        //update_screen_v1();
      }
      else if (current_mesure.type_capteur == 'T'){
        temp = current_mesure.mesure ;
        printf("Le consomateur a consomé %.2f de type temp \n ",temp);
        //update_screen_v1();
      }
      else if (current_mesure.type_capteur == 'C'){
         co2 = current_mesure.mesure ; 
         printf("Le consomateur a consomé %.2f de type C02 \n ",co2);
         //update_screen_v1();
       }

      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(250));
  
  }
  vTaskDelete(NULL); 
}




Ticker sensorDataTicker;

void sendSensorData() {
  jsonDoc.clear();
  jsonDoc["temperature"] = temp;
  jsonDoc["humidity"] = humidite;
  jsonDoc["co2"] = co2;

  String jsonString;
  serializeJson(jsonDoc, jsonString);
  ws.textAll(jsonString);
}


void setup() {
  setup_dht();

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  // Setup WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Start server
  server.begin();
  
  // Setup periodic sensor data sending
  sensorDataTicker.attach(1, sendSensorData); // Send data every 1 second

  SerialSensor.begin(9600, SERIAL_8N1, RXESP_PIN, TXESP_PIN);
  
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  s1 = xSemaphoreCreateCounting( TAILLE_MAX, TAILLE_MAX );
  s2 = xSemaphoreCreateCounting( TAILLE_MAX, 0 );
  mutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore( vProducteurTemperature, "ProducteurTemp", 10000, NULL, 1, NULL , 0 ); 
  xTaskCreatePinnedToCore( vProducteurHumidite, "ProducteurHumidite", 10000, NULL, 1, NULL , 0 );
  xTaskCreatePinnedToCore(vUARTReceiver, "UARTReceiver", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore( vConsomateur, "Consomateur", 10000, NULL, 1 ,NULL ,  0 );

}

void loop() {
  vTaskDelay(portMAX_DELAY);  
}
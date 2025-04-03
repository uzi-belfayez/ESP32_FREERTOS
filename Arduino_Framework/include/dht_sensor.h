#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHTesp.h"
#include <Ticker.h>

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

// Function declarations
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();
bool initTemp();

// External declarations for global variables
extern DHTesp dht;
extern TaskHandle_t tempTaskHandle;
extern Ticker tempTicker;
extern ComfortState cf;
extern bool tasksEnabled;
extern int dhtPin;

#endif // DHT_SENSOR_H
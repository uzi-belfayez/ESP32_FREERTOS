#include "dht_sensor.h"

// Global variables
DHTesp dht;
TaskHandle_t tempTaskHandle = NULL;
Ticker tempTicker;
ComfortState cf;
bool tasksEnabled = false;
int dhtPin = 17;

bool initTemp() {
    byte resultValue = 0;
    // Initialize temperature sensor
    dht.setup(dhtPin, DHTesp::DHT22);
    Serial.println("DHT initiated");

    // Start task to get temperature
    xTaskCreatePinnedToCore(
        tempTask,                       /* Function to implement the task */
        "tempTask ",                    /* Name of the task */
        4000,                           /* Stack size in words */
        NULL,                           /* Task input parameter */
        5,                              /* Priority of the task */
        &tempTaskHandle,                /* Task handle. */
        1);                             /* Core where the task should run */

    if (tempTaskHandle == NULL) {
        Serial.println("Failed to start task for temperature update");
        return false;
    } else {
        // Start update of environment data every 20 seconds
        tempTicker.attach(20, triggerGetTemp);
    }
    return true;
}

void triggerGetTemp() {
    if (tempTaskHandle != NULL) {
        xTaskResumeFromISR(tempTaskHandle);
    }
}

void tempTask(void *pvParameters) {
    Serial.println("tempTask loop started");
    while (1) // tempTask loop
    {
        if (tasksEnabled) {
            // Get temperature values
            getTemperature();
        }
        // Got sleep again
        vTaskSuspend(NULL);
    }
}

bool getTemperature() {
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    TempAndHumidity newValues = dht.getTempAndHumidity();
    // Check if any reads failed and exit early (to try again).
    if (dht.getStatus() != 0) {
        Serial.println("DHT11 error status: " + String(dht.getStatusString()));
        return false;
    }

    float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
    float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
    float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

    String comfortStatus;
    switch(cf) {
        case Comfort_OK:
            comfortStatus = "Comfort_OK";
            break;
        case Comfort_TooHot:
            comfortStatus = "Comfort_TooHot";
            break;
        case Comfort_TooCold:
            comfortStatus = "Comfort_TooCold";
            break;
        case Comfort_TooDry:
            comfortStatus = "Comfort_TooDry";
            break;
        case Comfort_TooHumid:
            comfortStatus = "Comfort_TooHumid";
            break;
        case Comfort_HotAndHumid:
            comfortStatus = "Comfort_HotAndHumid";
            break;
        case Comfort_HotAndDry:
            comfortStatus = "Comfort_HotAndDry";
            break;
        case Comfort_ColdAndHumid:
            comfortStatus = "Comfort_ColdAndHumid";
            break;
        case Comfort_ColdAndDry:
            comfortStatus = "Comfort_ColdAndDry";
            break;
        default:
            comfortStatus = "Unknown:";
            break;
    };

    Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
    return true;
}

void producteur_hum(){

}

void producteur_temp(){
    
}
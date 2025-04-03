#include "dht_sensor.h"

void setup() {
    Serial.begin(115200);
    Serial.println("DHT ESP32 example with tasks");
    initTemp();
    // Signal end of setup() to tasks
    tasksEnabled = true;
}

void loop() {
    if (!tasksEnabled) {
        // Wait 2 seconds before retrying
        delay(2000);
        tasksEnabled = true;
    }
    yield();
}
#include "dht_sensor.h"
#include <Wire.h>
#include "SSD1306Wire.h"
#include "images.h"

/*
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
}*/

#define SDA GPIO_NUM_5
#define SCL GPIO_NUM_4

SSD1306Wire display(0x3c, SDA, SCL);


void setup() {
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop() {
  display.clear();

  // Draw XBM image at (x=48, y=16)
  display.drawXbm(0, 0, image_width, image_height, sensor_screen_bits);

  display.display();
  delay(2000); // Wait before next refresh
}
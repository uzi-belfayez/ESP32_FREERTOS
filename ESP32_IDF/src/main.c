#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>


#define TRIGGER_PIN GPIO_NUM_33
#define ECHO_PIN GPIO_NUM_32
#define SOUND_SPEED 0.0343 // cm per microsecond
#define DETECTION_DISTANCE 50 // cm

#define TXD_PIN 17
#define RXD_PIN 16
#define UART_NUM UART_NUM_2
#define BUF_SIZE (128)

// #define TXESP_PIN 1
// #define RXESP_PIN 3
// #define UART_ESP UART_NUM_0

#define PWM_PIN GPIO_NUM_4 

static const char *TAG = "MH-Z19B";

SemaphoreHandle_t uart_mutex;

volatile int64_t start_time = 0;
volatile int64_t end_time = 0;
volatile bool measurement_done = false;
int object_detected = 0 ;


// Interrupt Service Routine (ISR) for Echo pin (ultrasonic sensor)
static void IRAM_ATTR echo_isr_handler(void *arg) {
    if (gpio_get_level(ECHO_PIN) == 1) {
        start_time = esp_timer_get_time(); 
    } else {
        end_time = esp_timer_get_time(); 
        measurement_done = true;
    }
}

// Function to trigger the ultrasonic sensor
void trigger_sensor() {
    gpio_set_level(TRIGGER_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2)); // 2 ms delay
    gpio_set_level(TRIGGER_PIN, 1);
    ets_delay_us(10); // 10 µs pulse
    gpio_set_level(TRIGGER_PIN, 0);
}

void hcsr04_task(void *pvParameter) {
    while (1) {
        trigger_sensor();

        if (measurement_done) {
            measurement_done = false;
            int64_t duration = end_time - start_time;
            float distance = (duration * SOUND_SPEED) / 2;

            printf("Distance: %.2f cm\n", distance);

            if (distance <= DETECTION_DISTANCE) {
                const char* output = "Object detected at 50 cm!\n";
                char uart_msg[64];

                snprintf(uart_msg, sizeof(uart_msg), "%s", output);

                if (xSemaphoreTake(uart_mutex, pdMS_TO_TICKS(100))) {
                    uart_write_bytes(UART_NUM, uart_msg, strlen(uart_msg));
                    xSemaphoreGive(uart_mutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}



// void co2_task(void *pvParameters) {
//     uint8_t request[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; 
//     uint8_t response[9];

//     while (1) {
//         uart_write_bytes(UART_NUM, (const char*)request, sizeof(request)); 
//         vTaskDelay(pdMS_TO_TICKS(100)); 

//         int len = uart_read_bytes(UART_NUM, response, sizeof(response), pdMS_TO_TICKS(200));
//         if (len == 9 && response[0] == 0xFF && response[1] == 0x86) {
//             int co2_ppm = (response[2] << 8) | response[3]; // Extract CO₂ value
//             ESP_LOGI(TAG, "CO₂ Concentration: %d ppm", co2_ppm);
//         } else {
//             ESP_LOGW(TAG, "Invalid response");
//         }

//         vTaskDelay(pdMS_TO_TICKS(4000)); 
//     }
// }

// reading CO2 sensor with UART 
// void co2_task(void *pvParameters) {
//     uint8_t request[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; 
//     uint8_t response[9];
//     char send_buffer[32];  

//     while (1) {
//         uart_write_bytes(UART_NUM, (const char*)request, sizeof(request)); 
//         vTaskDelay(pdMS_TO_TICKS(100)); 

//         int len = uart_read_bytes(UART_NUM, response, sizeof(response), pdMS_TO_TICKS(200));
//         if (len == 9 && response[0] == 0xFF && response[1] == 0x86) {
//             int co2_ppm = (response[2] << 8) | response[3]; // Extract CO₂ value
//             ESP_LOGI(TAG, "CO₂ Concentration: %d ppm", co2_ppm);

//             // Send over UART_ESP
//             snprintf(send_buffer, sizeof(send_buffer), "CO2:%d ppm\r\n", co2_ppm);
//             uart_write_bytes(UART_ESP, send_buffer, strlen(send_buffer));
//         } else {
//             ESP_LOGW(TAG, "Invalid response");
//         }

//         vTaskDelay(pdMS_TO_TICKS(4000)); 
//     }
// }


// reading CO2 sensor with PWM
void read_co2_pwm(void *arg) {
    while (1) {
        uint64_t start_time, high_time, low_time;

        // Wait for rising edge
        while (gpio_get_level(PWM_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        start_time = esp_timer_get_time();

        // Wait for falling edge
        while (gpio_get_level(PWM_PIN) == 1) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        high_time = esp_timer_get_time() - start_time;

        // Wait for rising edge again (LOW end)
        while (gpio_get_level(PWM_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        low_time = esp_timer_get_time() - (start_time + high_time);

        // Validate pulse width
        if (high_time < 2000 || low_time < 2000) {
            ESP_LOGE(TAG, "Invalid pulse readings: High = %llu us, Low = %llu us", high_time, low_time);
        } else {
            // Calculate CO2 concentration 
            float co2_ppm = 2000 * (high_time - 2000) / (high_time + low_time - 4000);
            ESP_LOGI(TAG, "CO₂ Concentration: %f ppm", co2_ppm);

            // Format and send 
            char uart_msg[64];
            int len = snprintf(uart_msg, sizeof(uart_msg), "%f", co2_ppm);
            if (xSemaphoreTake(uart_mutex, pdMS_TO_TICKS(100))) {
                uart_write_bytes(UART_NUM, uart_msg, len);
                xSemaphoreGive(uart_mutex);
            }
            
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



void app_main() {

    uart_mutex = xSemaphoreCreateMutex();

    // ---------------------------------------------- ULTRASONIC SENSOR CONFIG -----------------------------------------------
    // Configure Trigger Pin as Output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TRIGGER_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Configure Echo Pin as Input with Interrupt
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io_conf);

    // Install GPIO ISR Service and Attach ISR Handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ECHO_PIN, echo_isr_handler, NULL);

    // Create a FreeRTOS Task to Handle Distance Measurement
    xTaskCreate(hcsr04_task, "hcsr04_task", 2048, NULL, 5, NULL);


    // ---------------------------------------------- SENDING DATA CONFIG -----------------------------------------------

    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);


    // Create FreeRTOS Task for CO2 Sensor
    //xTaskCreate(co2_task, "co2_task", 2048, NULL, 5, NULL);

    xTaskCreate(read_co2_pwm, "read_co2_pwm", 4096, NULL, 5, NULL);

}
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

// My components
#include "gpio_driver.h"
#include "ir_sensor.h"
#include "motor_control.h"
#include "rpi_connector.h"
#include "robot_orchestrator.h"

// My Macros
#define IR_SENSOR_1_PIN GPIO_NUM_1
#define IR_SENSOR_2_PIN GPIO_NUM_2
#define IR_SENSOR_3_PIN GPIO_NUM_3
#define IR_SENSOR_4_PIN GPIO_NUM_4

#define RX_BUFFER_SIZE 256

static const char* TAG = "Main";

static void rpi_uart_task(void *pvParameters)
{
    uint8_t rx_buffer[RX_BUFFER_SIZE];

    while (1) {
        // Block up to 100 ms waiting for incoming data from RPi
        int bytes_received = rpi_uart_receive(rx_buffer, sizeof(rx_buffer) - 1, 100);

        if (bytes_received > 0) {
            rx_buffer[bytes_received] = '\0';

            // Trim trailing \r or \n for clean ESP log output
            while (bytes_received > 0 && 
                  (rx_buffer[bytes_received - 1] == '\n' || rx_buffer[bytes_received - 1] == '\r')) {
                rx_buffer[--bytes_received] = '\0';
            }

            ESP_LOGI(TAG, "Received from RPi (%d bytes): %s", bytes_received, rx_buffer);

            // Respond back to RPi with matching format + newline
            char ack_msg[RX_BUFFER_SIZE + 16];
            int ack_len = snprintf(ack_msg, sizeof(ack_msg), "ESP32_ACK: %s\n", rx_buffer);

            rpi_uart_send(ack_msg, ack_len);
        }
    }
}

void app_main(void)
{   
    ESP_LOGI(TAG, "Autonomous AI Vacuum Robot main program starting");
    esp_err_t ret; // Return var

    // Initialize motor control subsystem
    ret = motor_control_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize motor control: %s", esp_err_to_name(ret));
    }

    // Init IR Sensor 1
    ir_sensor_data_t irSensor1Data;
    ret = ir_sensor_init(IR_SENSOR_1_PIN, &irSensor1Data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured with IR Sensor 1 init\n Error: %s\n", esp_err_to_name(ret));
    }

    // Initialize the UART peripheral, pins, and driver
    esp_err_t err = rpi_uart_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UART to RPi: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "UART initialized successfully. Starting communication task...");

    // Create a dedicated FreeRTOS task for UART communication
    xTaskCreate(
        rpi_uart_task,      // Task function
        "rpi_uart_task",    // Task name
        4096,               // Stack depth in words
        NULL,               // Task parameters
        5,                  // Priority
        NULL                // Task handle
    );

    // Start robot orchestrator task
    ret = robot_orchestrator_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start robot orchestrator: %s", esp_err_to_name(ret));
    }
}
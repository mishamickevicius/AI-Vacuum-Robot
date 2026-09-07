#include "rpi_connector.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const char* TAG = "RPiConnector";

#define UART_BUFFER_SIZE (1024 * 2)
#define BAUDRATE 115200
#define UART_PORT_NUM UART_NUM_1
#define TX_PIN 17
#define RX_PIN 18

QueueHandle_t uart_queue = NULL;


esp_err_t install_uart_drivers()
{
    esp_err_t ret = uart_driver_install(
            UART_PORT_NUM,    // Port num
            UART_BUFFER_SIZE, // RX buffer size
            UART_BUFFER_SIZE, // TX buffer size
            15,               // Queue size
            &uart_queue,      // Uart Queue refrence
            0                 // Interrupt flags
        );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in install_uart_drivers: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t set_uart_params()
{
    uart_config_t uart_config = {
        .baud_rate = BAUDRATE,
        .data_bits = UART_DATA_8_BITS, // Num of transmitted bits
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    esp_err_t ret = uart_param_config(UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in set_uart_params: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t set_uart_pins()
{
    esp_err_t ret = uart_set_pin(UART_PORT_NUM, TX_PIN, RX_PIN, -1, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in set_uart_pins: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t rpi_uart_init()
{
    esp_err_t err;
    if ((err = set_uart_params()) != ESP_OK) return err;
    if ((err = set_uart_pins()) != ESP_OK) return err;
    if ((err = install_uart_drivers()) != ESP_OK) return err;
    // To fix and high impedence issues
    gpio_pullup_en(RX_PIN);
    gpio_pulldown_dis(RX_PIN);
    
    return ESP_OK;
}

/**
 * @brief Wrapper for uart_write_bytes.
 * 
 * @param data Pointer to the buffer to transmit.
 * @param len  Number of bytes to write.
 * @return int Number of bytes pushed to TX FIFO/ring buffer, or -1 on failure.
 */
int rpi_uart_send(const void *data, size_t len)
{
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "rpi_uart_send: Invalid arguments (NULL pointer or zero length)");
        return -1;
    }

    int bytes_written = uart_write_bytes(UART_PORT_NUM, data, len);
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "rpi_uart_send: Failed to write bytes to UART peripheral");
        return -1;
    }

    return bytes_written;
}

/**
 * @brief Wrapper for uart_read_bytes with timeout handling.
 * 
 * @param buf         Pointer to destination buffer.
 * @param max_len     Maximum number of bytes to read into buf.
 * @param timeout_ms  Time to block waiting for data in milliseconds.
 * @return int        Number of bytes read, 0 on timeout, or -1 on driver error.
 */
int rpi_uart_receive(void *buf, size_t max_len, uint32_t timeout_ms)
{
    if (buf == NULL || max_len == 0) {
        ESP_LOGE(TAG, "rpi_uart_receive: Invalid arguments (NULL buffer or zero max_len)");
        return -1;
    }

    TickType_t ticks_to_wait = (timeout_ms == portMAX_DELAY) 
                               ? portMAX_DELAY 
                               : pdMS_TO_TICKS(timeout_ms);

    int bytes_read = uart_read_bytes(UART_PORT_NUM, buf, max_len, ticks_to_wait);
    if (bytes_read < 0) {
        ESP_LOGE(TAG, "rpi_uart_receive: Driver error while reading UART");
        return -1;
    }

    return bytes_read;
}
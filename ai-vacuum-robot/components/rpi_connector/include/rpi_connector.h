#ifndef RPI_CONNECTOR
#define RPI_CONNECTOR

#include "esp_err.h"

esp_err_t install_uart_drivers();
esp_err_t set_uart_params();
esp_err_t set_uart_pins();
esp_err_t rpi_uart_init();
int rpi_uart_receive(void *buf, size_t max_len, uint32_t timeout_ms);
int rpi_uart_send(const void *data, size_t len);

#endif
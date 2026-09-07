#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>

esp_err_t set_gpio_to_output(int pinNum);
esp_err_t set_gpio_to_input(int pinNum, bool pullDownEn, bool pullUpEn, gpio_int_type_t intrType);
esp_err_t set_gpio(int pinNum, int value);
bool get_gpio_value(int pinNum);

#endif

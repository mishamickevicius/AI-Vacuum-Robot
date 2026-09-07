#include "gpio_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "GPIOTest";


/*
Standard GPIO Input model: 
1. Configure (Set to input if needed to listen for signals)
2. Listen for interrupts(When the specific event occurs, signal change, 
use an interrupt with Interrupt Service Routine(ISR))
3. Add to a queue(Using RTOS)
4. Process the messages in queue using a function/task
5. Repeat


Standard GPIO Output model: 
1. Configure (Get to output mode, typically disable interrupts)
2. Determine logic using functions/code
3. Repeat if needed
*/

// This function will be used by the main program to set the value of a GPIO pin
// Returns 1 if successful. 0 otherwise;
esp_err_t set_gpio(int pinNum, int value)
{
    // Check if GPIO pin is valid
    if (!GPIO_IS_VALID_GPIO(pinNum)) {
        ESP_LOGE("GPIO_DRIVER", "Invalid GPIO pin number: %d", pinNum);
        return ESP_ERR_INVALID_ARG;
    }

   esp_err_t ret = gpio_set_level(pinNum, value);
   // Check for errors
   if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error has occurred with set_gpio (pin %d, value %d). Error: %s",
             pinNum, (int)value, esp_err_to_name(ret)); // Cast 'value' to int for logging
    return ret;
   } else {
    return ESP_OK;
   }
}

// This function will set a specified gpio pin to output mode with pull_down enabled
esp_err_t set_gpio_to_output(int pinNum)
{
    // Check if GPIO pin is valid
    if (!GPIO_IS_VALID_GPIO(pinNum)) {
        ESP_LOGE("GPIO_DRIVER", "Invalid GPIO pin number: %d", pinNum);
        return ESP_ERR_INVALID_ARG;
    }


    // First set up config structure
    gpio_config_t output_config = {};
    output_config.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    output_config.mode = GPIO_MODE_OUTPUT; // Set to output mode
    output_config.pull_down_en = GPIO_PULLDOWN_ENABLE; // Enable pull down
    output_config.pull_up_en = GPIO_PULLUP_DISABLE; // Disable pull up
    // 1ULL creates a mask and << is a left shift
    output_config.pin_bit_mask = (1ULL << pinNum); // Select which pin to set

    esp_err_t ret = gpio_config(&output_config);

    if (ret == ESP_OK) {
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error has occured with set_gpio_to_output\n Error: %s\n", esp_err_to_name(ret));
        return ret;  // Return the error given
    }
}

esp_err_t set_gpio_to_input(int pinNum, bool pullDownEn, bool pullUpEn, gpio_int_type_t intrType)
{
    // Check if GPIO pin is valid
    if (!GPIO_IS_VALID_GPIO(pinNum)) {
        ESP_LOGE("GPIO_DRIVER", "Invalid GPIO pin number: %d", pinNum);
        return ESP_ERR_INVALID_ARG;
    }

    // Very similar to output mode function
    gpio_config_t input_config = {};
    input_config.intr_type = intrType;  // Configurable interrupt type
    input_config.pin_bit_mask = (1ULL << pinNum);
    input_config.mode = GPIO_MODE_INPUT;
    input_config.pull_down_en = pullDownEn; // Configurable
    input_config.pull_up_en = pullUpEn; // Configurable

    esp_err_t ret = gpio_config(&input_config);

    if (ret == ESP_OK) {
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error has occured with set_gpio_to_input\n Error: %s\n", esp_err_to_name(ret));
        return ret;  // Return the error given
    }
}

// This function will be a wrapper for the esp32 api function
bool get_gpio_value(int pinNum) 
{
    // Check if GPIO pin is valid
    if (!GPIO_IS_VALID_GPIO(pinNum)) {
        ESP_LOGE("GPIO_DRIVER", "Invalid GPIO pin number: %d", pinNum);
        return ESP_ERR_INVALID_ARG;
    }

    return (bool)gpio_get_level(pinNum);
}

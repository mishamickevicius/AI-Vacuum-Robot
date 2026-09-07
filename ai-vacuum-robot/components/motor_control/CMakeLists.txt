idf_component_register(
    SRCS "src/motor_control.c"

    INCLUDE_DIRS "include"

    REQUIRES
        "freertos"
        "driver"
        "gpio_driver"
        "esp_driver_mcpwm"
)
# Register the 'main' component
idf_component_register(
    # Only main.c is compiled by the 'main' component now.
    # gpio_driver.c is handled by the gpio_driver component.
    SRCS "main.c" "src/robot_orchestrator.c"

    # Any include directories for headers *specific to main*.
    INCLUDE_DIRS "." "include"

    # Declare that the 'main' component requires your 'gpio_driver' component.
    # This ensures gpio_driver is compiled and its public headers are visible to main.
    REQUIRES
        "gpio_driver"
        "ir_sensor"
        "motor_control"
        "rpi_connector"
        # List any other ESP-IDF components main.c directly uses, e.g.:
        # nvs_flash

)
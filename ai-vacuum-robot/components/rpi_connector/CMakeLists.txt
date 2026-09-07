idf_component_register(
    SRCS "src/rpi_connector.c"

    INCLUDE_DIRS "include"

    REQUIRES
        "freertos"
        "driver"
        "esp_driver_uart"
        "esp_driver_gpio"
)
# Register the gpio_driver component
idf_component_register(
    # Source files for this component.
    # Paths are relative to this CMakeLists.txt file.
    SRCS "src/gpio_driver.c"

    # Public include directories for this component.
    # Headers in 'include/' will be visible to other components that REQUIRE gpio_driver.
    INCLUDE_DIRS "include"

    # Declare dependencies of this component on other ESP-IDF components.
    # Your gpio_driver.c uses ESP-IDF's GPIO driver and FreeRTOS.
    REQUIRES
        "esp_driver_gpio"    # For ESP-IDF's GPIO API (gpio_set_level, gpio_config, etc.)
        "freertos"  # If you use FreeRTOS types or functions directly in gpio_driver.c
        "driver"
        # If gpio_driver.c used esp_log for logging, you would add:
        # esp_log
)

# Optional: If you had private headers within 'src/' that only gpio_driver.c uses,
# but no other components should see, you could add:
# PRIV_INCLUDE_DIRS "src"
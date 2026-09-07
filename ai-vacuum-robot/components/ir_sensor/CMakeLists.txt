idf_component_register(
    # Source files for this component
    SRCS "src/ir_sensor.c"

    # Public include directories for component.
    # Headers in 'include/' will be visible to other components that REQUIRE gpio_driver.
    INCLUDE_DIRS "include"

    # Declare dependencies of this component
    REQUIRES
        "freertos"
        "driver"
    PRIV_REQUIRES
        "gpio_driver"
)
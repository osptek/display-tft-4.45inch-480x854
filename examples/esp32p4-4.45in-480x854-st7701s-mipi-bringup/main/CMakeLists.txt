idf_component_register(SRCS "esp32p4-4.45in-480x854-st7701s-mipi-bringup.c"
                            "bsp_display.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd esp_driver_gpio esp_hw_support)

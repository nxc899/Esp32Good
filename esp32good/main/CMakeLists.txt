idf_component_register(SRCS "my_lcd.c" "main.c"
                       PRIV_REQUIRES spi_flash esp_driver_spi esp_driver_gpio
                       INCLUDE_DIRS ".")

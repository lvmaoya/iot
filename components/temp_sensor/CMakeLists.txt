idf_component_register(SRCS "temp_sensor_main.c"
                        REQUIRES 
                            spi_flash 
                            esp_wifi 
                            esp_event 
                            nvs_flash 
                            driver 
                       INCLUDE_DIRS ".")

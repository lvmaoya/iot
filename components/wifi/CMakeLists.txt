idf_component_register(SRCS "wifi_config.c"
                    REQUIRES 
                        esp_wifi 
                        esp_event 
                        nvs_flash 
                        driver 
                        wifi_provisioning
                        mbedtls
                    INCLUDE_DIRS ".")

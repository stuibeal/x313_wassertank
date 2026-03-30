idf_component_register(SRCS "main.c"
							"valve.c"
							"pumpe.c"
                            "bluetooth_spp/bluetooth_spp.c"
                            "gpio_handler/gpio_handler.c"
                            "tl_136_sensor/tl_136_sensor.c"
                            "x313_led_strip/x313_led_strip.c"
                            
                    PRIV_REQUIRES bt nvs_flash vfs esp_adc
                    REQUIRES driver
                    INCLUDE_DIRS ".")

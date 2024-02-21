set(PRIVREQ esp_timer)

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    list(APPEND PRIVREQ esp_adc)
else() 
    list(APPEND PRIVREQ esp_adc_cal)
endif()

idf_component_register(SRCS "out_8574.c" "out_gpio.c" "iot_out.c"
                        INCLUDE_DIRS "."
                        REQUIRES pcf
                        PRIV_REQUIRES ${PRIVREQ})

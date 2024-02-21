
#ifndef _IOT_BUTTON_8574_H_
#define _IOT_BUTTON_8574_H_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"



#ifdef __cplusplus
extern "C" {
#endif

#include "../pcf/i2cdev.h"

typedef struct {
    uint8_t pin_num;
    i2c_dev_t *device;
    uint8_t active_level;
} button_8574_config_t;

esp_err_t button_8574_init(const button_8574_config_t *config);
esp_err_t button_8574_deinit(int gpio_num);
uint8_t button_8574_get_key_level(void *gpio_num);


#ifdef __cplusplus
}
#endif


#endif
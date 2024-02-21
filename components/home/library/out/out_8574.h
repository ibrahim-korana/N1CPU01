
#ifndef _IOT_OUT_8574_H_
#define _IOT_OUT_8574_H_

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
    uint8_t reverse;
} out_8574_config_t;

esp_err_t out_8574_init(const out_8574_config_t *config);
esp_err_t out_8574_deinit(int gpio_num);

uint8_t out_8574_get_level(void *hardware);
uint8_t out_8574_set_level(void *hardware, uint8_t level);
uint8_t out_8574_toggle_level(void *hardware);

uint8_t out_read(i2c_dev_t *dev, uint8_t num, out_8574_config_t *cfg);
void out_write(i2c_dev_t *dev, uint8_t num, uint8_t level, out_8574_config_t *cfg );


#ifdef __cplusplus
}
#endif


#endif
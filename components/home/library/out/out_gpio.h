
#ifndef _IOT_OUT_GPIO_H_
#define _IOT_OUT_GPIO_H_

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t gpio_num;
} out_gpio_config_t;

esp_err_t out_gpio_init(const out_gpio_config_t *config);
esp_err_t out_gpio_deinit(int gpio_num);
uint8_t out_gpio_get_level(void *gpio_num);
uint8_t out_gpio_set_level(void *gpio_num, uint8_t level);
uint8_t out_gpio_toggle_level(void *gpio_num);


#ifdef __cplusplus
}
#endif

#endif
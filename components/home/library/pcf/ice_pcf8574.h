
#ifndef __ICEPCF8574_H__
#define __ICEPCF8574_H__

#include <stddef.h>
#include "i2cdev.h"
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t pcf8574_init_desc(i2c_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t pcf8574_free_desc(i2c_dev_t *dev);
esp_err_t pcf8574_port_read(i2c_dev_t *dev, uint8_t *val);
esp_err_t pcf8574_port_write(i2c_dev_t *dev, uint8_t value);
esp_err_t pcf8574_pin_write(i2c_dev_t *dev, uint8_t pin, uint8_t val);
uint8_t pcf8574_pin_read(i2c_dev_t *dev, uint8_t pin);



#ifdef __cplusplus
}
#endif

#endif /* __ICEPCF8574_H__ */

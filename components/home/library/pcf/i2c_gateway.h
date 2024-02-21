#ifndef __I2C_GATEWAY_H__
#define __I2C_GATEWAY_H__

#include <stddef.h>
#include "i2cdev.h"
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gateway_init_desc(i2c_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t gateway_free_desc(i2c_dev_t *dev);
esp_err_t gateway_read(i2c_dev_t *dev, const uint8_t *out, uint8_t outlen, uint8_t *val, uint8_t vallen);
esp_err_t gateway_write(i2c_dev_t *dev, uint8_t *val, uint8_t vallen);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_GATEWAY_H__ */

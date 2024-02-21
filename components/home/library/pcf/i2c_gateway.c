
#include <esp_err.h>
#include "esp_idf_lib_helpers.h"
#include "i2cdev.h"
#include "i2c_gateway.h"

#define I2C_FREQ_HZ 100000
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t gateway_init_desc(i2c_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);
    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.master.clk_speed = I2C_FREQ_HZ;
    return i2c_dev_create_mutex(dev);
}

esp_err_t gateway_free_desc(i2c_dev_t *dev)
{
    CHECK_ARG(dev);
    return i2c_dev_delete_mutex(dev);
}

esp_err_t gateway_read(i2c_dev_t *dev, const uint8_t *out, uint8_t outlen, uint8_t *val, uint8_t vallen)
{
    CHECK_ARG(dev && val);
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev,i2c_dev_read(dev, out, outlen, val, vallen));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t gateway_write(i2c_dev_t *dev, uint8_t *val, uint8_t vallen)
{
    CHECK_ARG(dev);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, val, vallen));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}
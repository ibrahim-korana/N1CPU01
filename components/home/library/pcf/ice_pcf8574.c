

#include <esp_err.h>
#include "esp_idf_lib_helpers.h"
#include "i2cdev.h"
#include "ice_pcf8574.h"

#define I2C_FREQ_HZ 100000

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define BV(x) (1 << (x))

static esp_err_t read_port(i2c_dev_t *dev, uint8_t *val)
{
    CHECK_ARG(dev && val);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev,i2c_dev_read(dev, NULL, 0, val, 1));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

static esp_err_t write_port(i2c_dev_t *dev, uint8_t val)
{
    CHECK_ARG(dev);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &val, 1));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t pcf8574_init_desc(i2c_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);
    CHECK_ARG(addr & 0x20);

    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(dev);
}

esp_err_t pcf8574_free_desc(i2c_dev_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(dev);
}

esp_err_t pcf8574_port_read(i2c_dev_t *dev, uint8_t *val)
{
    return read_port(dev, val);
}

esp_err_t pcf8574_port_write(i2c_dev_t *dev, uint8_t val)
{
    return write_port(dev, val);
}

esp_err_t pcf8574_pin_write(i2c_dev_t *dev, uint8_t pin, uint8_t val)
{
    uint8_t dt=0;
    pcf8574_port_read(dev,&dt);
    if (val == 0)
    {
        dt &= ~(1 << pin);
    } else {
        dt |= (1 << pin);
    }
    return write_port(dev, dt);
}

uint8_t pcf8574_pin_read(i2c_dev_t *dev, uint8_t pin)
{
    uint8_t dt=0;
    pcf8574_port_read(dev,&dt);
    return (dt & (1 << (uint8_t)pin)) > 0;
}

#include "out_8574.h"
#include "../pcf/ice_pcf8574.h"

static const char *TAG = "8574 out";

//out_8574_config_t *out_8574_config;


#define GPIO_OUT_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

esp_err_t out_8574_init(const out_8574_config_t *config)
{
    GPIO_OUT_CHECK(NULL != config, "Pointer of config is invalid", ESP_ERR_INVALID_ARG);
    //out_8574_config=config;
    return ESP_OK;
}

esp_err_t out_8574_deinit(int gpio_num)
{
    return ESP_OK;
}

uint8_t out_read(i2c_dev_t *dev, uint8_t num, out_8574_config_t *cfg)
{
    uint8_t aa = pcf8574_pin_read(dev,num);
    if (cfg->reverse==1) aa=!aa;
  return !aa;
}
void out_write(i2c_dev_t *dev, uint8_t num, uint8_t level,out_8574_config_t *cfg )
{
    uint8_t aa = level;
    if (cfg->reverse==1) aa=!aa;
    pcf8574_pin_write(dev,num,!aa);
}

uint8_t out_8574_get_level(void *hardware)
{
    out_8574_config_t *cfg = (out_8574_config_t *)hardware;
    return out_read(cfg->device,cfg->pin_num, cfg);
}

uint8_t out_8574_set_level(void *hardware, uint8_t level)
{
    out_8574_config_t *cfg = (out_8574_config_t *)hardware;
    out_write(cfg->device,cfg->pin_num,level, cfg);
    return out_read(cfg->device,cfg->pin_num, cfg);
}

uint8_t out_8574_toggle_level(void *hardware)
{
    out_8574_config_t *cfg = (out_8574_config_t *)hardware;
    uint8_t level = out_read(cfg->device,cfg->pin_num, cfg);
    out_write(cfg->device,cfg->pin_num,!level, cfg);
    return !level;
}

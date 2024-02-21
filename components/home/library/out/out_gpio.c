#include "esp_log.h"
#include "driver/gpio.h"
#include "out_gpio.h"

static const char *TAG = "gpio out";

#define GPIO_OUT_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

esp_err_t out_gpio_init(const out_gpio_config_t *config)
{
    GPIO_OUT_CHECK(NULL != config, "Pointer of config is invalid", ESP_ERR_INVALID_ARG);

    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    gpio_conf.pin_bit_mask = (1ULL << config->gpio_num);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&gpio_conf);

    return ESP_OK;
}

esp_err_t out_gpio_deinit(int gpio_num)
{
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_DISABLE,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&gpio_conf);
    return ESP_OK;
}

uint8_t out_gpio_get_level(void *gpio_num)
{
    return gpio_get_level((gpio_num_t)gpio_num);
}

uint8_t out_gpio_set_level(void *gpio_num, uint8_t level)
{
    gpio_set_level((gpio_num_t)gpio_num, level);
    return level;
}

uint8_t out_gpio_toggle_level(void *gpio_num)
{
    uint8_t level = out_gpio_get_level(gpio_num);
    gpio_set_level((gpio_num_t)gpio_num, !level);
    return !level;
}
#ifndef _IOT_OUT_H_
#define _IOT_OUT_H_

#include "out_gpio.h"
#include "out_8574.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *out_handle_t;
typedef void (* out_cb_t)(void *, void *);

typedef enum {
    OUT_TYPE_GPIO,
    OUT_TYPE_EXPANDER,
} out_type_t;

typedef struct {
    out_type_t type;                          
    union {
        out_gpio_config_t gpio_out_config;
        out_8574_config_t pcf8574_out_config;
    }; 
} out_config_t;

out_handle_t iot_out_create(const out_config_t *config);
esp_err_t iot_out_register_cb(out_handle_t out_handle, out_cb_t cb, void *usr_data);
uint8_t iot_out_set_level(out_handle_t out_handle, uint8_t level);
bool iot_out_get_level(out_handle_t out_handle);
uint8_t iot_out_set_toggle(out_handle_t out_handle);
bool iot_out_set_disable(out_handle_t out_handle, bool disable);
bool iot_get_get_disable(out_handle_t out_handle);
void *iot_out_get_user_data(out_handle_t handle);
void iot_out_list(void);




#ifdef __cplusplus
}
#endif

#endif
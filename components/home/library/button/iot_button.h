#ifndef _IOT_BUTTON_H_
#define _IOT_BUTTON_H_

#include "button_adc.h"
#include "button_gpio.h"
#include "button_8574.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (* button_cb_t)(void *, void *);
typedef void *button_handle_t;

typedef enum {
    BUTTON_PRESS_DOWN = 0,
    BUTTON_PRESS_UP,
    BUTTON_PRESS_REPEAT,
    BUTTON_SINGLE_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_HOLD,
    BUTTON_ALL_EVENT,
    BUTTON_EVENT_MAX,
    BUTTON_NONE_PRESS,   
} button_event_t;

typedef enum {
    BUTTON_TYPE_GPIO,
    BUTTON_TYPE_ADC,
    BUTTON_TYPE_EXPANDER,
} button_type_t;

typedef struct {
    button_type_t type;                          
    union {
        button_gpio_config_t gpio_button_config;
        button_adc_config_t adc_button_config;  
        button_8574_config_t pcf8574_button_config;
    }; 
} button_config_t;

button_handle_t iot_button_create(const button_config_t *config);
esp_err_t iot_button_delete(button_handle_t btn_handle);
esp_err_t iot_button_register_cb(button_handle_t btn_handle, button_event_t event, button_cb_t cb, void *usr_data);
esp_err_t iot_button_unregister_cb(button_handle_t btn_handle, button_event_t event);
button_event_t iot_button_get_event(button_handle_t btn_handle);
uint8_t iot_button_get_repeat(button_handle_t btn_handle);
bool iot_button_set_disable(button_handle_t btn_handle, bool disable);
bool iot_button_get_disable(button_handle_t btn_handle);
void *iot_button_get_user_data(button_handle_t btn_handle);

void iot_button_start(void);
void iot_button_list(void);



#ifdef __cplusplus
}
#endif

#endif


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "iot_out.h"

static const char *TAG = "out";

#define OUT_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

typedef struct Out {
    uint8_t         (*hal_set_level)(void *hardware_data, uint8_t level);
    uint8_t         (*hal_get_level)(void *hardware_data);
    uint8_t         (*hal_toggle_level)(void *hardware_data);
    void            *hardware_data;
    void            *usr_data;
    out_cb_t        cb;
    out_type_t      type;
    bool            disable;
    struct Out      *next;
} out_dev_t;

static out_dev_t *go_head_handle = NULL;

void *iot_out_get_user_data(out_handle_t handle)
{
    out_dev_t *btn = (out_dev_t *) handle;
    return btn->usr_data;

}
static out_dev_t *out_create_com(uint8_t (*hal_set_level)(void *hardware_data, uint8_t level), 
                                    uint8_t (*hal_get_level)(void *hardware_data),
                                    uint8_t (*hal_toggle_level)(void *hardware_data),
                                    void *hardware_data)
{
    OUT_CHECK(NULL != hal_set_level, "Function pointer is invalid", NULL);
    OUT_CHECK(NULL != hal_get_level, "Function pointer is invalid", NULL);
    OUT_CHECK(NULL != hal_toggle_level, "Function pointer is invalid", NULL);

    out_dev_t *out = (out_dev_t *) calloc(1, sizeof(out_dev_t));
    OUT_CHECK(NULL != out, "out memory alloc failed", NULL);
    out->hardware_data = hardware_data;
    out->hal_set_level = hal_set_level;
    out->hal_get_level = hal_get_level;
    out->hal_toggle_level = hal_toggle_level;
    out->disable = false;

    /** Add handle to list */
    out->next = go_head_handle;
    go_head_handle = out;

    return out;
}

esp_err_t iot_out_register_cb(out_handle_t out_handle, out_cb_t cb, void *usr_data)
{
    OUT_CHECK(NULL != out_handle, "Pointer of handle is invalid", ESP_ERR_INVALID_ARG);
    out_dev_t *btn = (out_dev_t *) out_handle;
    btn->cb = cb;
    btn->usr_data = usr_data;
    return ESP_OK;
}


uint8_t iot_out_set_level(out_handle_t out_handle, uint8_t level)
{
    OUT_CHECK(NULL != out_handle, "Pointer of handle is invalid", 0);
    out_dev_t *out = (out_dev_t *) out_handle;
    uint8_t ret = 255;
    if (!out->disable)
      {
        ret = out->hal_set_level(out->hardware_data,level);
        if (out->cb!=NULL) out->cb(out, out->usr_data);
      }
    return ret;
}

bool iot_out_get_level(out_handle_t out_handle)
{
   OUT_CHECK(NULL != out_handle, "Pointer of handle is invalid", 0);
   out_dev_t *out = (out_dev_t *) out_handle; 
   uint8_t ret = out->hal_get_level(out->hardware_data);
   return (bool)ret;
}

uint8_t iot_out_set_toggle(out_handle_t out_handle)
{
    OUT_CHECK(NULL != out_handle, "Pointer of handle is invalid", 0);
    out_dev_t *out = (out_dev_t *) out_handle;
    uint8_t ret = 255;
    if (!out->disable)
      {
        ret = out->hal_toggle_level(out->hardware_data);
        if (out->cb!=NULL) out->cb(out, out->usr_data);
      }
    return ret;
}


out_handle_t iot_out_create(const out_config_t *config)
{
    esp_err_t ret = ESP_OK;
    out_dev_t *out = NULL;
    switch (config->type) {
    case OUT_TYPE_GPIO: {
        const out_gpio_config_t *cfg = &(config->gpio_out_config);
        ret = out_gpio_init(cfg);
        OUT_CHECK(ESP_OK == ret, "gpio out init failed", NULL);
        out = out_create_com(out_gpio_set_level,out_gpio_get_level,out_gpio_toggle_level, (void *)cfg->gpio_num);
    } break;
    case OUT_TYPE_EXPANDER: {
        const out_8574_config_t *cfg = &(config->pcf8574_out_config);
        ret = out_8574_init(cfg);
        OUT_CHECK(ESP_OK == ret, "8574 out init failed", NULL);
        out = out_create_com(out_8574_set_level,out_8574_get_level,out_8574_toggle_level, (void *)cfg);
    } break;
    default:
        ESP_LOGE(TAG, "Unsupported out type");
        break;
    }
    OUT_CHECK(NULL != out, "out create failed", NULL);
    out->type = config->type;
    return (out_handle_t)out;
}

bool iot_out_set_disable(out_handle_t out_handle, bool disable)
{
    out_dev_t *out = (out_dev_t *) out_handle;
    out->disable = disable;
    return out->disable;
}
bool iot_get_get_disable(out_handle_t out_handle)
{
    out_dev_t *out = (out_dev_t *) out_handle;
    return out->disable;
}

uint8_t rev_out_pin_convert(uint8_t pcfno, uint8_t pcfpin)
{
    switch(pcfno)
      {
        case 1 :{
            switch(pcfpin)
            {
                case 0: {return 1;break;}
                case 1: {return 2;break;}
                case 2: {return 3;break;}
                case 3: {return 4;break;}
                case 7: {return 5;break;}
                case 6: {return 6;break;}
            }
            break;
        }
        case 0 :{
            switch(pcfpin)
            {
                case 1: {return 7;break;}
                case 2: {return 8;break;}
                case 3: {return 9;break;}
                case 4: {return 10;break;}
                case 5: {return 11;break;}
                case 6: {return 12;break;}
            } 
            break;
        }
      }
  return 0;
}

void iot_out_list(void)
{
   uint16_t number = 0;
    out_dev_t *target = go_head_handle;
    while (target) {
        target = target->next;
        number++;
    } 

    printf("OUT SAYISI %d\n",number);
    number=0;
   target = go_head_handle;
    while (target) {
        printf("OUT  %d Type = %d ",number++,target->type);
        if (target->type==1) 
           {
              out_8574_config_t *cfg = (out_8574_config_t *) target->hardware_data;
              printf("PCF ADRES %X pin num = %d rev=%d Klemens=%d",cfg->device->addr, cfg->pin_num,cfg->reverse,rev_out_pin_convert(cfg->device->addr-0x20, cfg->pin_num));
           }
        printf("\n");   
        target = target->next;
       
    } 

}
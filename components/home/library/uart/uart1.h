
#ifndef _UART_H
#define _UART_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "math.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "freertos/event_groups.h"

#include "../core/core.h"


typedef struct {
  uint8_t uart_num;
  uint8_t dev_num;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint8_t atx_pin;
  uint8_t arx_pin;
  int baud;
} UART_config_t;


#define UART_READ_TIMEOUT 3
#define UART_PATTERN_CHR_NUM 2

#define BUF_SIZE 512

#define UART_DEBUG false

class UART {
    public:
        UART(void) {};
        UART(UART_config_t *cfg, uart_transmisyon_callback_t cb) {
            initialize(cfg, cb);
        };
        void initialize(UART_config_t *cfg, uart_transmisyon_callback_t cb);     
        ~UART(void) {};
        return_type_t Sender(const char *data, uint8_t receiver, bool response=false);  
        uint8_t get_uart_num(void) {return uart_number;}
        uint8_t get_device_id(void) {return device_id;}
        bool paket_decode(uint8_t *data);
        bool is_busy(void) {return busy;}
        
        uart_transmisyon_callback_t callback=NULL;

      
        gpio_num_t ARX = GPIO_NUM_NC;
        gpio_num_t ATX = GPIO_NUM_NC;

        QueueHandle_t u_queue;
        QueueHandle_t send_queue;

        char *paket_buffer=NULL;
        uint8_t paket_sender=0;
        bool paket_response=false;
        
        gpio_num_t PING_LED;
        
        TaskHandle_t SenderTask = NULL;
        TaskHandle_t ReceiverTask = NULL;
        bool busy = false;
        uint8_t send_paket_error;
        xSemaphoreHandle send_paket_sem = NULL;
        xSemaphoreHandle callback_sem = NULL;
        xSemaphoreHandle wait_sem = NULL;
        

//232 844 7648

    protected:
        uint8_t device_id=0;
        uint8_t uart_number = 0;

        uint16_t paket_counter = 0;
        uint16_t paket_length = 0;
        uint32_t paket_header = 0;

                
        static void _event_task(void *param);
        static void _sender_task(void *arg);
        static void _callback_task(void *arg);

        bool ATX_Stat(uint8_t drm);
        bool ARX_Stat(uint8_t drm);
        
        return_type_t _Sender(Data_t *param);        
};

#endif
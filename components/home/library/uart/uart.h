
#ifndef _UART_H
#define _UART_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
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

#define PING_TIMEOUT  5*1000000
#define PONG_TIMEOUT  8*1000000


//typedef void (*uart_transmisyon_callback_t)(char *data);
//typedef void (*ping_reset_callback_t)(uint8_t counter);

typedef struct {
  uint8_t uart_num;
  uint8_t dev_num;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint8_t atx_pin;
  uint8_t arx_pin;
  uint8_t int_pin;
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
        uart_port_t get_uart_num(void) {return uart_number;}
        uint8_t get_device_id(void) {return device_id;}
        bool paket_decode(uint8_t *data);
        bool is_busy(void) {return busy;}
        
        uart_transmisyon_callback_t callback=NULL;

        
        void ping_start(uint8_t dev, ping_reset_callback_t cb, int led);
        void ping_timer_restart(void);
        uint8_t ping_device=0;
        ping_reset_callback_t ping_reset_callback = NULL;
        gpio_num_t PING_LED;
        uint8_t ping_error_counter = 0;
        bool ping(uint8_t dev);

        uint8_t pong_device=0;
        void pong_start(uint8_t dev,ping_reset_callback_t cb,int ld);
        ping_reset_callback_t pong_reset_callback = NULL;
        void pong_timer_restart(void);
        uint8_t pong_error_counter = 0;

      
        gpio_num_t ARX = GPIO_NUM_NC;
        gpio_num_t ATX = GPIO_NUM_NC;
        gpio_num_t INT = GPIO_NUM_NC;

        QueueHandle_t u_queue;
        QueueHandle_t send_queue;

        char *paket_buffer=NULL;
        uint8_t paket_sender=0;
        bool paket_response=false;
        
        TaskHandle_t SenderTask = NULL;
        TaskHandle_t ReceiverTask = NULL;
        bool busy = false;
        uint8_t send_paket_error;
        SemaphoreHandle_t send_paket_sem = NULL;
        SemaphoreHandle_t callback_sem = NULL;
        SemaphoreHandle_t wait_sem = NULL;
        SemaphoreHandle_t ping_sem = NULL;
        SemaphoreHandle_t ack_wait_sem = NULL;
        bool ping_stat = false;

        QueueHandle_t IQ;
        

//232 844 7648

    protected:
        uint8_t device_id=0;
        uart_port_t uart_number = (uart_port_t)0;

        uint16_t paket_counter = 0;
        uint16_t paket_length = 0;
        uint32_t paket_header = 0;
        uint16_t ping_counter = 0;

        esp_timer_handle_t ping_tim;
        static void ping_timer_callback(void *arg);
        static void pong_timer_callback(void *arg);
        esp_timer_handle_t pong_tim;
      
                
        static void _event_task(void *param);
        static void _sender_task(void *arg);
        static void _callback_task(void *arg);

        bool ATX_Stat(uint8_t drm);
        bool ARX_Stat(uint8_t drm);
        void ping_timer_start(void);
        void ping_timer_stop(void);
        void pong_timer_start(void);
        void pong_timer_stop(void);
        
        return_type_t _Sender(Data_t *param);        
};

#endif
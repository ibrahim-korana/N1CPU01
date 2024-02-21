
#ifndef _485_H
#define _485_H

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

#define RS485_MASTER 254
//#define RS485_PC 250

#define RS485_BROADCAST 255

#define RS485_READ_TIMEOUT 3
#define PATTERN_CHR_NUM 2

#define BAUD_RATE 57600
#define BUF_SIZE 512

#define RS485_DEBUG false

#define PING_TIMEOUT  5*1000000
#define PONG_TIMEOUT  8*1000000
#define DELAY_TIMEOUT 500000

#define Xb0 (1<<0)
#define Xb1 (1<<1)
#define Xb2 (1<<2)
#define Xb3 (1<<3)
#define Xb4 (1<<4)
#define Xb5 (1<<5)
#define Xb6 (1<<6)
#define Xb7 (1<<7)
#define Xb8 (1<<8)
#define Xb9 (1<<9)


class RS485 {
    public:
        RS485(void) {};
        RS485(RS485_config_t *cfg, uart_transmisyon_callback_t cb, int baud) {
            initialize(cfg, cb, baud);
        };
        void initialize(RS485_config_t *cfg, uart_transmisyon_callback_t cb, int baud);     
        ~RS485(void) {};
        return_type_t Sender(const char *data, uint8_t receiver, bool response=false);  
        uint8_t get_uart_num(void) {return uart_number;}
        uint8_t get_device_id(void) {return device_id;}
        bool paket_decode(uint8_t *data);
        bool is_busy(void) {return wait;}
        void ping_start(uint8_t dev, ping_reset_callback_t cb, int ld);
        void send_fin(uint8_t receiver);
        
        uart_transmisyon_callback_t callback=NULL;

        uint8_t OE_PIN = 255;
        char *paket_buffer=NULL;
        uint8_t paket_sender=0;
        bool paket_response=false;
        rs_mode_t Mode = RECEIVER;

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

          QueueHandle_t u_queue;
          QueueHandle_t s_queue;
          QueueHandle_t p_queue;
          
          
        EventGroupHandle_t RSEvent;
        TaskHandle_t SenderTask = NULL;
        TaskHandle_t ReceiverTask = NULL;
        

//232 844 7648
        void ping_timer_restart(void);

    protected:
        uint8_t device_id=RS485_MASTER;
        bool wait = false; 
        uint8_t uart_number = 0;

        uint16_t paket_counter = 0;
        uint16_t ping_counter = 0;
        uint16_t paket_length = 0;
        uint32_t paket_header = 0;

        esp_timer_handle_t ping_tim;
        esp_timer_handle_t pong_tim;
                
        static void _event_task(void *param);
        static void _sender_task(void *arg);
        static void _callback_task(void *arg);
        
        return_type_t _Sender(Data_t *param);
        static void ping_timer_callback(void *arg);
        static void pong_timer_callback(void *arg);
        void ping_timer_start(void);
        void ping_timer_stop(void);
        void pong_timer_start(void);
        void pong_timer_stop(void);
        
};

#endif

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

#define PING_TIMEOUT  5*1000000
#define PONG_TIMEOUT  8*1000000
#define RS485_BROADCAST 255
#define RS485_MASTER 254

#define RS485_READ_TIMEOUT 3
#define RS485_PATTERN_CHR_NUM 2

#undef BUF_SIZE
#define BUF_SIZE 512

class RS485 {
    public:
        RS485(void) {};
        RS485(RS485_config_t *cfg, rs485_callback_t cb, rs485_callback_t bcb, gpio_num_t ld) {
            initialize(cfg, cb, bcb, ld);
        };
        void initialize(RS485_config_t *cfg, rs485_callback_t cb,rs485_callback_t bcb, gpio_num_t ld);     
        ~RS485(void) {};
        return_type_t Sender(const char *data, uint8_t receiver, bool response=false);  
        uart_port_t get_uart_num(void) {return uart_number;}
        uint8_t get_device_id(void) {return device_id;}
        bool paket_decode(uint8_t *data);
        bool is_busy(void) {return busy;}
        bool ping(uint8_t dev);

        uint8_t OE_PIN = 255;
        
        rs485_callback_t callback=NULL;
        rs485_callback_t broadcast_callback = NULL;

        QueueHandle_t u_queue;
        QueueHandle_t send_queue;
        gpio_num_t led;

        char *paket_buffer=NULL;
        uint8_t paket_sender=0;
        uint8_t paket_receiver=0;
        bool paket_response=false;
        
        TaskHandle_t SenderTask = NULL;
        TaskHandle_t ReceiverTask = NULL;
        bool busy = false;
        uint8_t send_paket_error;
        SemaphoreHandle_t send_paket_sem = NULL;
        SemaphoreHandle_t callback_sem = NULL;
        SemaphoreHandle_t wait_sem = NULL;
        SemaphoreHandle_t ack_sem = NULL;
        SemaphoreHandle_t ping_sem = NULL;
        bool ack_ok=false;
        bool ping_ok=false;

    protected:
        uint8_t device_id=0;
        uart_port_t uart_number = (uart_port_t)0;

        uint16_t paket_counter = 0;
        uint16_t paket_length = 0;
        uint32_t paket_header = 0;

        static void _event_task(void *param);
        static void _sender_task(void *arg);
        static void _callback_task(void *arg);
        
        return_type_t _Sender(Data_t *param);        
};

#endif
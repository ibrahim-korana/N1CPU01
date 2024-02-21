#ifndef __WIFI_NOW_H__
#define __WIFI_NOW_H__

//Version 2.0

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "math.h"

#include "esp_wifi_types.h"
#include "esp_interface.h"

#include "iptool.h"
#include "core.h"
#include "cihazlar.h"

#define SEND_LEN 200
#define NPAKET_SIZE 190

#define QUEUE_SIZE 5

#define MODE_STATION true

#if MODE_STATION==true
    #define ESPNOW_WIFI_MODE WIFI_MODE_STA
    #define ESPNOW_WIFI_IF   (wifi_interface_t)ESP_IF_WIFI_STA
#else
    #define ESPNOW_WIFI_MODE WIFI_MODE_AP
    #define ESPNOW_WIFI_IF   (wifi_interface_t)ESP_IF_WIFI_AP
#endif

#define ESPNOW_CHANNEL 1
#define MAC_LEN 6

enum {
    DATA_BROADCAST,
    DATA_UNICAST,
    DATA_MAX,
    DATA_PING,
    DATA_PONG,
};

typedef struct {
    uint8_t type;                         //Broadcast or unicast ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint8_t sender;
    uint8_t receiver;
    uint8_t total_pk;
    uint8_t current_pk;   
    uint16_t total_len;  
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[MAC_LEN];   //MAC address of destination device.
} send_param_t;

typedef struct {
    uint8_t type;                         //Broadcast or unicast ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint8_t sender;
    uint8_t receiver;
    uint8_t total_pk;
    uint8_t current_pk;
    uint8_t len;
    uint16_t total_len;    
    uint8_t payload[0];                   //Real payload of ESPNOW data.
}  __attribute__((packed)) data_t;

typedef struct {
    uint8_t mac[MAC_LEN];
    uint8_t *data;
    int data_len;
} receive_cb_t;

typedef struct {
    uint8_t mac[MAC_LEN];
    esp_now_send_status_t status;
} send_cb_t;

typedef struct {
    uint32_t header;
    uint8_t id;
    char *data;
} receive_pk_t;


void EspNOW_init(uint8_t id);
esp_err_t EspNOW_Send(const char *data, uint8_t id );
esp_err_t EspNOW_Broadcast(const char *data);
void EspNOW_set_led(gpio_num_t ld);
void EspNOW_set_callback(rs485_callback_t cb);
void EspNOW_set_broadcast_callback(rs485_callback_t cb);
void EspNOW_set_cihazlar(Cihazlar *chz);
bool EspNOW_ping(uint8_t id);
bool EspNOW_is_busy(void);



#endif
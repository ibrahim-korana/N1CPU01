#ifndef _HOME_CORE_H
#define _HOME_CORE_H

#include <stdio.h>
#include "lwip/ip_addr.h"
#include "esp_event.h"


#define NETWORK_ERROR     0x001 
#define NETWORK_STA       0x002
#define NETWORK_STA_ERR   0x004
#define NETWORK_AP        0x008
#define NETWORK_AP_ERR    0x010
#define NETWORK_ETH       0x020
#define NETWORK_ETH_ERR   0x040
#define NETWORK_MQTT      0x080
#define NETWORK_MQTT_ERR  0x100


#define MAX_DEVICE 50
#define MAX_FUNCTION 200

typedef enum {
    HOME_WIFI_DEFAULT = 0,
    HOME_WIFI_AP,     //Access point oluşturur
    HOME_WIFI_STA,    //Access station oluşturur
    HOME_WIFI_AUTO,   //Access station oluşturur. AP bağlantı hatası oluşursa AP oluşturur 
    HOME_ETHERNET,    //SPI ethernet
    HOME_NETWORK_DISABLE,//Wifi kapatır   
} home_wifi_type_t;

typedef enum {
    IP_DEFAULT = 0,
    STATIC_IP, 
    DYNAMIC_IP, 
} home_ipstat_type_t;

typedef struct {
    uint8_t home_default; 
    home_wifi_type_t wifi_type;
    uint8_t wifi_ssid[32];        //AP Station de kullanılacak SSID
    uint8_t wifi_pass[32];        //AP Station de kullanılacak Şifre 
    home_ipstat_type_t ipstat;
    uint8_t ip[17];               //Static ip 
    uint8_t netmask[17];          //Static netmask
    uint8_t gateway[17];          //Static gateway
    
    uint8_t sta_open;
    uint32_t home_ip;
    uint32_t home_netmask;
    uint32_t home_gateway;
    uint32_t home_broadcast;
    uint8_t mac[17];
    uint8_t espnow;
    uint8_t dns[17]; 
    uint8_t backup_dns[17]; 
    uint8_t console_ip[17]; 
    uint8_t update_server[17];
    uint8_t upgrade;
    uint8_t version[32];
} home_network_config_t;

typedef struct {
    uint8_t home_default;
    uint8_t device_name[32];
    uint8_t device_id;
    uint8_t mqtt_server[32];
    uint8_t license[32];
    uint8_t mqtt_keepalive;
    uint8_t start_value;
    uint8_t http_start;
    uint8_t tcp_start;
	
    uint8_t comminication;
    uint8_t reset_servisi;
    uint8_t aktif_senaryo;
    uint8_t blok;
    uint8_t daire;
    uint8_t location;
    uint16_t project_id;
    uint8_t random_mac;
    uint8_t rawmac[4];
    uint8_t ping_time;
    uint8_t reset_counter;
    uint8_t set_sntp;
} home_global_config_t;

struct remote_reg_t {
    uint8_t device_id;
    char name[20];
    uint8_t register_id;
    uint8_t register_device_id;
    uint8_t yer;
};


typedef enum {
  TR_NONE = 0,
  TR_UDP = 1,
  TR_SERIAL,
  TR_ESPNOW,
  TR_WIRE,
  TR_LOCAL,
  TR_TCP,
  TR_GATEWAY,
} transmisyon_t;


typedef enum {
  RET_OK = 0,
  RET_NO_ACK,
  RET_TIMEOUT,
  RET_COLLESION,
  RET_NO_CLIENT,
  RET_HARDWARE_ERROR,
  RET_BUSY,
  RET_RESPONSE,
} return_type_t;

typedef struct device_register {
    uint8_t device_id;
    uint32_t ip;
    transmisyon_t transmisyon;
    uint8_t active;
    uint8_t oldactive;
    uint8_t function_count;
    char mac[14];
    int socket;
    uint8_t mac0[6];
    void *next;
} device_register_t;

struct function_reg_t {
    char name[20];
    char auname[20];
    uint8_t device_id;
    uint8_t register_id;
    uint8_t register_device_id;
    uint8_t prg_loc;
    transmisyon_t transmisyon;
};

typedef struct {
    bool stat;
    float temp;
    float set_temp;
    uint64_t color; 
    uint8_t status;
    uint8_t ircom[32];
    uint8_t irval[32];
    uint8_t counter; 
    bool active;
    bool first;
} home_status_t;

typedef enum {
    PORT_NC = 0,
    PORT_INPORT,    //in
    PORT_INPULS,    //in
    PORT_OUTPORT,   //out
    PORT_SENSOR,    //in
    PORT_VIRTUAL,   //in
    PORT_FIRE,      //in
    PORT_WATER,     //in
    PORT_GAS,       //in
    PORT_EMERGENCY, //in
} port_type_t;

typedef enum {
    ACTION_INPUT = 0,
    ACTION_OUTPUT, 
} port_action_type_t;

struct port_action_s
{   port_action_type_t action_type;
    void *port;
} ;
typedef struct port_action_s port_action_t;

struct function_prop_t {
    char name[20];
    char uname[20];
    uint8_t device_id;    
    uint8_t register_id;
    uint8_t register_device_id;
    bool active;
    bool virtual_device;
    bool registered;
    transmisyon_t location;
};

typedef struct prg_location {
    char name[20];
    uint8_t page;
    void *next;
} prg_location_t;

typedef enum {
  PAKET_NORMAL = 0,
  PAKET_PING,
  PAKET_PONG,
  PAKET_ACK,
  PAKET_RESPONSE,
  PAKET_START,
  PAKET_FIN,
} RS485_paket_type_t;

typedef struct {
  RS485_paket_type_t paket_type:4;
  uint8_t paket_ack:1; 
  uint8_t frame_ack:1;
} RS485_flag_t;

typedef struct {
  uint8_t sender;
  uint8_t receiver;
  RS485_flag_t flag;
  uint8_t total_pk;
  uint8_t current_pk;
  uint8_t id;
  uint8_t data_len;
} RS485_header_t;

typedef struct {
  uint8_t uart_num;
  uint8_t dev_num;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint8_t oe_pin;
  int baud;
} RS485_config_t;

typedef enum {
  SENDER = 0,
  RECEIVER,
} rs_mode_t;

typedef struct 
{
    uint8_t *data;
    uint8_t receiver;
    RS485_paket_type_t paket_type;
    void *self;
} Data_t;

typedef struct 
{
    RS485_header_t *header;
    uint8_t *data;
} Paket_t;
/*
      EVENT
*/
typedef struct {
	uint8_t state;
} led_events_data_t;

ESP_EVENT_DECLARE_BASE(LED_EVENTS);

enum {
	LED_EVENTS_ETH,
	LED_EVENTS_WIFI,
	LED_EVENTS_MQTT,
    LED_EVENTS_PING
};
/*
         UTIL FUNCTIONS
*/
port_type_t port_type_convert(char * aa);
void port_type_string(port_type_t tp,char *aa);
void transmisyon_string(transmisyon_t tt,char *aa);
void transmisyon_type_convert(home_wifi_type_t tp,char *aa);


/*
         CALLBACK
*/

typedef void (*rs485_callback_t)(char *data, uint8_t sender, transmisyon_t transmisyon);
typedef void (*transmisyon_callback_t)(char *data, uint8_t sender, transmisyon_t transmisyon, void *response, bool responsevar);
typedef void (*uart_transmisyon_callback_t)(char *data);
typedef void (*ping_reset_callback_t)(uint8_t counter);

typedef void (*network_change_callback_t)(void *arg);
typedef void (*netconfig_change_callback_t)(home_network_config_t cnf);

typedef void (*port_callback_t)(void *arg, port_action_t action);
typedef void (*register_callback_t)(void *arg);
typedef void (*function_callback_t)(void *prt, home_status_t stat);
typedef void (*web_callback_t)(void);
typedef void (*default_callback_t)(void);
typedef void (*default_parcallback_t)(void *arg);

typedef void (*mqtt_callback_t)(char* topic, char* data, int topiclen, int datalen);
typedef void (*change_callback_t)(void *arg);


#endif
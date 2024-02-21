#ifndef _ETHERNET_J60_H
#define _ETHERNET_J60_H

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"

#include "freertos/event_groups.h"

#include "core.h"

#define ETH_CONNECTED_BIT BIT0
#define ETH_FAIL_BIT      BIT1

class Ethj60 {
    public:
      Ethj60() {};
      ~Ethj60() {};

      esp_err_t start(home_network_config_t cnf, home_global_config_t *gcnf);
      void set_connect_bit(void) {
    	                             xEventGroupSetBits(Event, ETH_CONNECTED_BIT);
                                   led_events_data_t ld={};
                                   ld.state = 0;
                                   ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
                                   Active=true;
                                 }
      void set_fail_bit(void) {
    	                        xEventGroupSetBits(Event, ETH_FAIL_BIT);
                              led_events_data_t ld={};
                              ld.state = 1;
                              ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
    	                        Active=false;
                              }
      bool is_active(void){return Active;}
      static esp_err_t set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type);
      void reset(void);

      esp_eth_handle_t eth_handle = NULL;
      esp_err_t _start(void);
      eth_enc28j60_config_t enc28j60_config;
      esp_netif_t *eth_netif;
      esp_netif_config_t netif_cfg;
      esp_netif_ip_info_t info_t;
      esp_eth_mac_t *mac;
      
    private:
       EventGroupHandle_t Event;
       home_network_config_t mConfig;
       home_global_config_t *gConfig;
       bool Active=false;
    protected:  
        
};

#endif

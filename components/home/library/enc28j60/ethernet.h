#ifndef _ETHERNET_H
#define _ETHERNET_H

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

class Ethernet {
    public:
        Ethernet() {};
        ~Ethernet() {};
        esp_err_t start(home_network_config_t cnf, home_global_config_t *gcnf);
        void set_connect_bit(void) {
    	                            xEventGroupSetBits(Event, ETH_CONNECTED_BIT);
                                    ld.state = 0;
                                    ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
                                 }
      void set_fail_bit(void) {
    	                        xEventGroupSetBits(Event, ETH_FAIL_BIT);
                                ld.state = 1;
                                ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
                              }
        void reset(void);                      

    private:
       EventGroupHandle_t Event;
       home_network_config_t mConfig;
       home_global_config_t *gConfig;

       esp_netif_t *eth_netif;
       /*
       eth_enc28j60_config_t enc28j60_config;
       esp_eth_mac_t *mac;
       esp_eth_phy_t *phy;
       esp_eth_config_t eth_config;
       */
       esp_netif_ip_info_t info_t;

       esp_eth_handle_t eth_handle = NULL;

       esp_err_t set_ip(void);
       esp_err_t set_dns(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type);
       void set_macid(esp_eth_mac_t *mc);
       
    protected:  

        const TickType_t xTicksToWait = 8000 / portTICK_PERIOD_MS;
        EventBits_t uxBits;
        led_events_data_t ld={};
};

#endif
#ifndef _UDP_BROADCAST_H
#define _UDP_BROADCAST_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/socket.h"
#include "netdb.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "../core/core.h"
#include "../iptool/iptool.h"
#include "../jsontool/jsontool.h"

#undef BUFFER_SIZE
#define BUFFER_SIZE 128


class UdpBroadcast {
    public:
      bool Start(
               home_network_config_t *netconfig, 
               home_global_config_t *globalconfig, 
               uint16_t sport,
               rs485_callback_t cb
               );
      esp_err_t Send(const char *data);
      void Command(const char *cm, uint8_t dev, uint8_t function_count);

      UdpBroadcast() {
        server_fatal_error = false;
      };
      ~UdpBroadcast(){
      };

      
      uint32_t subnetBroadcast = 0;
      int port = 0;
      IPAddr Adr = IPAddr();
      int Broadcast_Soket = -1;
      bool wait = false;

      //özel tanımlar
      //---------------------------
      SemaphoreHandle_t server_ready;
      bool server_fatal_error;
      const char *TAG = "UDP_SERVER";
      home_network_config_t *Net_Config; 
      home_global_config_t *Glo_Config; 
      rs485_callback_t callback=NULL;
    private:
    

      static void UDP_broadcast_listener(void *arg);
};   

#endif
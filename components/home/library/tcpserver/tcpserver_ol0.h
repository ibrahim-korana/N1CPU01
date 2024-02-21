#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

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
#include "../cihazlar/cihazlar.h"
#include "../jsontool/jsontool.h"

#define MAX_SOCKET 20
#define INVALID_SOCK (-1)
#define YIELD_TO_ALL_MS 50
#define BUFFER_SIZE 512

#define UDP_BROADCAST_PORT 7700

#define SERVER_MODE

class TcpServer {
    public:
      #ifdef SERVER_MODE
      bool Start(   home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      uint16_t udpport, 
                      transmisyon_callback_t cb,
                      Cihazlar *chz
                    );
      #endif   
      #ifdef CLIENT_MODE
      bool Start(   home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      uint16_t udpport, 
                      transmisyon_callback_t cb,
                      const char *srvip
                    );
      #endif                  
       
      esp_err_t Send(char *data, uint8_t id);

      esp_err_t Udp_Send(const char *data);
      void Broadcast_Command(const char *cm, uint8_t dev);

      TcpServer() {
        port = (char *)malloc(7);
        adres = (char *)malloc(17);
        server_fatal_error = false;
      };
      ~TcpServer(){
        free(port);
        free(adres);
      };

      
      
      //özel tanımlar
      //---------------------------
      #ifdef SERVER_MODE 
      xSemaphoreHandle server_ready;
      #endif
      char *adres;
      char *port;
      bool server_fatal_error;
      #ifdef SERVER_MODE
        const char *TAG = "TCP_SERVER";
        
      #elif CLIENT_MODE
        const char *TAG = "TCP_CLIENT";
      #else
        const char *TAG = "UDP_BROADCAST";   
      #endif

      int Broadcast_Soket = -1;
      Cihazlar *cihaz = NULL;
      
      home_network_config_t *net_config;
      home_global_config_t *dev_config;
      transmisyon_callback_t callback=NULL;
      uint16_t TcpPort = -1;
      uint16_t UdpPort = -1;
      tcpip_adapter_ip_info_t ip_info;
      uint32_t BroadcastAddr=0;

    private:
      static void tcp_server_task(void *arg); 
      static void log_socket_error(const char *tag, const int sock, const int err, const char *message);
      static int try_receive(const char *tag, const int sock, uint8_t * data, size_t max_len);
      static int socket_send(const char *tag, const int sock, const char * data, const size_t len);
      static void get_clients_address(struct sockaddr_storage *source_addr, char *address_str);

      static void UDP_broadcast_listener(void *arg);
};   

#endif
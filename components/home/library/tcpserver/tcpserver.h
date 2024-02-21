#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "core.h"

#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

typedef struct tcp_client {
    int sock;
    uint32_t ip;
    uint8_t active;
    tcp_client *next;
} tcp_client_t;

class TcpServer {
    public:
      TcpServer() {};
      ~TcpServer(){};
      bool start(uint16_t prt, transmisyon_callback_t cb);
      uint8_t Send(char *data);

      transmisyon_callback_t data_callback;
      uint16_t port;
      tcp_client_t *client_handle = NULL;
      void add_client(tcp_client_t *cln);
      tcp_client_t *find_client(tcp_client_t *cln);
      void list_client(void);
      void delete_client(int sock);

      uint8_t last_socket=0;
      bool sender_busy = false;

      
    private:
      static void server_task(void *param);
      static void data_transmit(const int sock, void *arg);
      static void wait_data(void *arg);  
};

#endif
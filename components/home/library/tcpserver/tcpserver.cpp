#include "tcpserver.h"

static const char *SOCK_TAG = "SOCKET_SERVER";



void TcpServer::wait_data(void *arg)
{
   TcpServer *mthis = (TcpServer *)arg; 
   uint8_t sck = mthis->last_socket;
   data_transmit(sck, arg);
   //shutdown(mthis->last_socket, 0);
   mthis->delete_client(sck);
   mthis->list_client();
   close(sck);
   vTaskDelete(NULL);
}

void TcpServer::data_transmit(const int sock, void *arg)
{
    int len;
    char *rx_buffer = (char *)malloc(512);
    TcpServer *mthis = (TcpServer *)arg;

    do {
        len = recv(sock, rx_buffer, 510, 2);
        if (len < 0) {
            ESP_LOGE(SOCK_TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(SOCK_TAG, "%d Connection closed",sock);            
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            if (len != 14) ESP_LOGI(SOCK_TAG, "Received %d bytes: %s", len, rx_buffer);
            if (mthis->data_callback!=NULL)
              {
                char *res = (char *)malloc(512);
                memset(res,0,511);
                mthis->data_callback(rx_buffer, sock, TR_TCP,(void *)res, false);
                if (strlen(res)>2) 
                {
                    strcat(res,"&");
                    send(sock, res, strlen(res), 0);
                }
                free(res);
              }
        }
    } while (len > 0);
    free(rx_buffer);
}

void TcpServer::server_task(void *arg)
{
    char addr_str[128];
    int addr_family = (int)AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    TcpServer *mthis = (TcpServer *)arg;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(mthis->port);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(SOCK_TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //ESP_LOGI(SOCK_TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(SOCK_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(SOCK_TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    //ESP_LOGI(SOCK_TAG, "Socket bound, port %d", mthis->port);

    err = listen(listen_sock, 3);
    if (err != 0) {
        ESP_LOGE(SOCK_TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }
    while (1) {

        ESP_LOGI(SOCK_TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(SOCK_TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCK_TAG,"Client %d socket connected.",sock); 
        
        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            uint32_t ip = (uint32_t)(((struct sockaddr_in *)&source_addr)->sin_addr).s_addr;
            tcp_client_t *target = mthis->client_handle;
            bool bulundu=false;
            while(target)
              {
                if (target->ip==ip && target->sock==sock)
                  {
                    target->active = 1;
                    bulundu = true;
                    break;
                  }
                target = (tcp_client_t *)target->next;
                if (bulundu) target=NULL;
              }
            if (!bulundu)
              {
                 tcp_client_t *cln = (tcp_client_t *)malloc(sizeof(tcp_client_t));
                 cln->sock = sock;
                 cln->active = 1;
                 cln->ip = ip;
                 cln->next = mthis->client_handle;
                 mthis->client_handle = cln;
              } 
            mthis->last_socket = sock;

        //mthis->list_client();    

        ESP_LOGI(SOCK_TAG, "Socket accepted ip address: %s", addr_str);

        //data_transmit(sock, arg);
        xTaskCreate(&wait_data,"wait_data",3072,arg,5,NULL);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);

}

uint8_t TcpServer::Send(char *data)
{
    uint8_t sz = 0; 
    if (sender_busy) return 0;
    sender_busy = true;
    tcp_client_t *target = client_handle;
    char *dt = (char *)malloc(strlen(data)+4);
    memset(dt,0,strlen(data)+4);
    strcpy(dt,data);
    strcat(dt,"&");
    ESP_LOGI(SOCK_TAG,"SEND DATA >> %s",dt); 
    while (target)
      {
        if (target->active==1) sz = send(target->sock, dt, strlen(dt), 0);
        target = (tcp_client_t *)target->next;
      } 
    free(dt);  
    sender_busy = false;  
    return sz;  
}

bool TcpServer::start(uint16_t prt,transmisyon_callback_t cb)
{
    data_callback = cb;
    port = prt;
    xTaskCreate(server_task, "tcp_server", 4096, (void*)this, 5, NULL);
    return true;
}

tcp_client_t *TcpServer::find_client(tcp_client_t *cln)
{
    tcp_client_t *target = client_handle;
    while (target)
      {
        printf("%lu %d %d\n",target->ip,target->active,target->sock);
        printf("%lu %d %d\n",cln->ip,cln->active,cln->sock);
        if (target->ip == cln->ip && 
            target->sock==cln->sock) return target;
        target = (tcp_client_t *)target->next;
      }
    return NULL;  
}

void TcpServer::list_client(void)
{
    tcp_client_t *target = client_handle;
    ESP_LOGI(SOCK_TAG,"SOKET LISTESI");
    ESP_LOGI(SOCK_TAG,"-------------");
    while (target)
      {
        if (target->active==1) ESP_LOGI(SOCK_TAG,"Socket %d Active %d",target->sock, target->active);
        target = (tcp_client_t *)target->next;
      }
}

void TcpServer::delete_client(int sock)
{
    tcp_client_t *target = client_handle;
    bool ex = false;
    while (target)
      {
        if (target->sock == sock && target->active==1)
        {
            target->active = 0;
            ex=true;
        }
        target = (tcp_client_t *)target->next;
        if (ex) target=NULL;
      } 
}

void TcpServer::add_client(tcp_client_t *cln)
{
    tcp_client_t *fn = find_client(cln);
    if (fn==NULL)
      {
        cln->next = client_handle;
        client_handle = cln;
        printf("eklendi\n");
      } else {
         fn->active = 1;
         printf("duzenlendi\n");
      }
      
}
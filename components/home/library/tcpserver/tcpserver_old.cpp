#include "tcpserver.h"

static const char *SOCK_TAG = "SOCKET_SERVER";

bool sender_busy = false;

void TcpServer::wait_data(void *arg)
{
   TcpServer *mthis = (TcpServer *)arg; 

   uint8_t sck = mthis->Baglanan->socket;
   data_transmit(sck, arg);

   if (mthis->Baglanan->socket==sck) mthis->Baglanan->socket = -1;
   close(sck);

  printf("Wait Data Exit socket %d\n",sck); 

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
            if (errno==104) ESP_LOGE(SOCK_TAG, "Client Baglantıyı kapatmış errno:%d", errno);
            else
            ESP_LOGE(SOCK_TAG, "DATA  during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(SOCK_TAG, "%d Connection closed",sock);            
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
           // ESP_LOGI(SOCK_TAG, "Received %d bytes: %s", len, rx_buffer);
            if (mthis->data_callback!=NULL)
              {
                char *res = (char *)malloc(512);
                memset(res,0,512);
                mthis->data_callback(rx_buffer, sock, TR_UDP,(void *)res, false);
                if (strlen(res)>2) 
                {
                    strcat(res,"&");
                    send(sock, res, strlen(res), 0);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                free(res);
              }
        }
    } while (len > 0);
    free(rx_buffer);
    rx_buffer=NULL;
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
        ESP_LOGE(SOCK_TAG, "SERV TASK Lıstener DEAD Unable to create socket: errno %d", errno);
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

    err = listen(listen_sock, 5);
    if (err != 0) {
        ESP_LOGE(SOCK_TAG, "LISTEN Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }
    while (1) {

        ESP_LOGI(SOCK_TAG, "%d Socket listening",mthis->port);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(SOCK_TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            uint32_t ip = (uint32_t)(((struct sockaddr_in *)&source_addr)->sin_addr).s_addr;
            char *mac =(char*)malloc(16);
            memset(mac,0,16);
            if (mthis->cihaz!=NULL) {
              mthis->cihaz->get_sta_mac(ip,mac);
              mthis->Baglanan=mthis->cihaz->cihazbul(mac);
              if (mthis->Baglanan!=NULL) {
                mthis->Baglanan->socket = sock;
                ESP_LOGI(SOCK_TAG,"Client %d socket connected.",sock); 
              }
            }

        if (mthis->Baglanan!=NULL) 
            xTaskCreate(&wait_data,"wait_data",3072,arg,5,NULL); 
        else
            close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);

}

uint8_t TcpServer::Send(char *data, uint8_t id)
{
   uint8_t sz = 0; 
  // if (sender_busy) return 0;
   sender_busy = true;
   device_register_t *target = cihaz->get_handle();
   while (target)
      {
        printf("SOKET %d=%d %d\n",target->device_id,id,target->socket);
        if (target->device_id==id && target->socket>0) 
          {
             char *dt = (char *)malloc(strlen(data)+4);
             strcpy(dt,data);
             strcat(dt,"&");
             printf("SOKET SEND %s\n",dt);
             sz = send(target->socket, dt, strlen(dt), 0); 
             vTaskDelay(100 / portTICK_PERIOD_MS);
             free(dt);
             break;        
          }
        target = (device_register_t *)target->next;
      } 
    sender_busy = false;  
    return sz;  
}

esp_err_t TcpServer::Udp_Send(const char *data)
{
    char *dt = (char *)malloc(strlen(data)+2);
    strcpy((char*)dt,data);
    strcat((char*)dt,"&");
    int datasize = strlen(dt);
    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    uint32_t subnetBroadcast = ip_info.ip.addr | ~(ip_info.netmask.addr);    
    static int iobAnnounce;
    const char broadcastEnable = 1;
    struct sockaddr_in servaddrAnnounce;
    iobAnnounce = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(iobAnnounce, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    memset(&servaddrAnnounce, 0, sizeof(servaddrAnnounce));
    servaddrAnnounce.sin_family = AF_INET;
    servaddrAnnounce.sin_port = htons(UDP_BROADCAST_PORT);
    servaddrAnnounce.sin_addr.s_addr = (subnetBroadcast);
    int sz=sendto(iobAnnounce,dt,datasize,0,(const struct sockaddr *) &servaddrAnnounce,sizeof(servaddrAnnounce));

    printf("send Broadcast %d=%d %d %s\n",sz,datasize,errno, dt);

    free(dt); 
    dt=NULL;
    close(iobAnnounce);    
    if (sz!=datasize) {return ESP_FAIL;}
  return ESP_OK;   
}

void TcpServer::Broadcast_Command(const char *cm, uint8_t dev)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", cm);
    cJSON_AddNumberToObject(root, "id", dev);   
    char *dat = cJSON_PrintUnformatted(root);
    if (Udp_Send(dat)==ESP_FAIL) ESP_LOGE(SOCK_TAG, "Udp Send ERROR");
    cJSON_free(dat);
    cJSON_Delete(root); 
}

void TcpServer::UDP_broadcast_listener(void *arg)
{
    char *rx_buffer = (char *)malloc(512);
    int addr_family = 0;
    int ip_protocol = 0;
    TcpServer *mthis = (TcpServer *)arg;
    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = mthis->config->home_broadcast;//inet_addr("192.168.7.255");//mthis->config->home_broadcast;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(UDP_BROADCAST_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        mthis->Broadcast_Soket = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (mthis->Broadcast_Soket < 0) {ESP_LOGE(SOCK_TAG, "Udp ERR");break;}

        int broadcast = 1;
        setsockopt(mthis->Broadcast_Soket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        if (bind(mthis->Broadcast_Soket, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) { ESP_LOGE(SOCK_TAG, "Udp ERR");break; }
        ESP_LOGI(SOCK_TAG, "%d UDP Broadcast Socket Listening", UDP_BROADCAST_PORT); 
        while (1) {
            struct sockaddr_storage source_addr; 
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(mthis->Broadcast_Soket, rx_buffer, 510, 0, (struct sockaddr *)&source_addr, &socklen);
            if (len < 0) { ESP_LOGE(SOCK_TAG, "Udp ERR"); break; }
            // Data received
            else {
                    rx_buffer[len] = 0; 
                    
                    if (rx_buffer[len-1]=='&') 
                      {
                        rx_buffer[len-1] = 0;
                        char addr_str[128]; 
                        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                        uint32_t ip = (uint32_t)(((struct sockaddr_in *)&source_addr)->sin_addr).s_addr;

                        cJSON *rcv = cJSON_Parse((char*)rx_buffer);
                        char *command = (char *)calloc(1,20);
                        uint8_t iid=0;
                        JSON_getstring(rcv,"com", command); 
                        JSON_getint(rcv,"id",&iid);
                        if (strcmp(command,"sinfo_ack")==0) {   
                            if (mthis->cihaz!=NULL) {
                              char *mac =(char*)malloc(16);
                              JSON_getstring(rcv,"mac", mac); 
                              mthis->cihaz->get_sta_mac(ip,mac);
                              device_register_t *Bag=mthis->cihaz->cihazbul(mac);
                              if (Bag!=NULL) {
                                Bag->device_id = iid; 
                              }
                              free(mac);
                            }                            
                        }
                        if (strcmp(command,"sinfo")==0) { 
                              if (mthis->cihaz!=NULL) {
                              char *mac =(char*)malloc(16);
                              JSON_getstring(rcv,"mac", mac); 
                              mthis->cihaz->get_sta_mac(ip,mac);
                              device_register_t *Bag=mthis->cihaz->cihazbul(mac);
                              if (Bag!=NULL) Bag->device_id = iid; 
                              free(mac);
                              }
                              mthis->Broadcast_Command("sinfo_ack",254); 
                              ESP_LOGI(SOCK_TAG,"%d  server info sended ",iid); 
                            }
                        cJSON_Delete(rcv);  
                      }

                    printf("UDP broadcast %s\n",rx_buffer); 

                 }
            vTaskDelay(10);
        }
        if (mthis->Broadcast_Soket != -1) {
            ESP_LOGE(SOCK_TAG, "Udp ERR");
            shutdown(mthis->Broadcast_Soket, 0);
            close(mthis->Broadcast_Soket);
        }
    }
    free(rx_buffer);
}

bool TcpServer::start(home_network_config_t *cnf, home_global_config_t *hcnf, uint16_t prt,transmisyon_callback_t cb)
{
    data_callback = cb;
    port = prt;
    config = cnf;
    dev_config = hcnf;
    xTaskCreate(server_task, "tcp_server", 4096, (void*)this, 5, NULL);
    xTaskCreate(UDP_broadcast_listener, "udp_listener", 4096, (void*)this, 5, NULL);
    return true;
}

tcp_client_t *TcpServer::find_client(tcp_client_t *cln)
{
    tcp_client_t *target = client_handle;
    while (target)
      {
        printf("%u %d %d\n",target->ip,target->active,target->sock);
        printf("%u %d %d\n",cln->ip,cln->active,cln->sock);
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
        ESP_LOGI(SOCK_TAG,"Socket %d Active %d",target->sock, target->active);
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
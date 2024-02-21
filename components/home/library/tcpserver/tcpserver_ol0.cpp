#include "tcpserver.h"

void TcpServer::get_clients_address(struct sockaddr_storage *source_addr, char *address_str)
{
    char *res = NULL;
    //printf("Kaynak %d\n",((struct sockaddr_in *)source_addr)->sin_addr.s_addr); 32bit adres
    if (source_addr->ss_family == PF_INET) {
        res = inet_ntoa_r(((struct sockaddr_in *)source_addr)->sin_addr, address_str, 14);
    }
    if (!res) {
        address_str[0] = '\0';
    }
}

int TcpServer::socket_send(const char *tag, const int sock, const char * data, const size_t len)
{
    int to_write = len;
    while (to_write > 0) {
        int written = send(sock, data + (len - to_write), to_write, 0);
        if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            log_socket_error(tag, sock, errno, "Gönderimde hata oluştu");
            return -1;
        }
        to_write -= written;
    }
    return len;
}

int TcpServer::try_receive(const char *tag, const int sock, uint8_t * data, size_t max_len)
{
    int len = recv(sock, data, max_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        if (errno == ENOTCONN) {
            ESP_LOGW(tag, "[sock=%d]: Soket kapatıldı", sock);
            return -2;  
        }
        log_socket_error(tag, sock, errno, "Soket kapatılmış");
        return -1;
    }
    return len;
}

void TcpServer::log_socket_error(const char *tag, const int sock, const int err, const char *message)
{
    ESP_LOGE(tag, "[sock=%d]: %s\n"
                  "error=%d: %s", sock, message, err, strerror(err));
}

#ifdef SERVER_MODE
void TcpServer::tcp_server_task(void *arg)
{
    TcpServer *mthis = (TcpServer *)arg;
    uint8_t *rx_buffer = (uint8_t *)malloc(BUFFER_SIZE);
    int err = 0, flags = 0;
    
    struct addrinfo hints = {};
        hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *address_info;
    int listen_sock = INVALID_SOCK;
    const size_t max_socks = MAX_SOCKET - 1;
    static int sock[MAX_SOCKET - 1];

    for (int i=0; i<max_socks; ++i) {
        sock[i] = INVALID_SOCK;
    }

    int res = getaddrinfo(mthis->adres, mthis->port, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(mthis->TAG, "%s için getaddrinfo() %d hatası oluşturdu addrinfo=%p", mthis->adres, res, address_info);
        goto error;
    }

    listen_sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);

    if (listen_sock < 0) {
        log_socket_error(mthis->TAG, listen_sock, errno, "Soket oluşturulamadı");
        goto error;
    }
    ESP_LOGI(mthis->TAG, "soket oluşturuldu");

    // Marking the socket as non-blocking
    flags = fcntl(listen_sock, F_GETFL);
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_socket_error(mthis->TAG, listen_sock, errno, "Soket non bloking olarak işaretlenemiyor");
        goto error;
    }

    err = bind(listen_sock, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) {
        log_socket_error(mthis->TAG, listen_sock, errno, "Soket adrese bind edilemedi");
        goto error;
    }
    ESP_LOGI(mthis->TAG, "%s:%s başlatıldı",mthis->adres,mthis->port);
    
    err = listen(listen_sock, 1);
    if (err != 0) {
        log_socket_error(mthis->TAG, listen_sock, errno, "Dinleme sırasında hata oluştu");
        goto error;
    }
    ESP_LOGI(mthis->TAG, "Socket dinleniyor");
    xSemaphoreGive(mthis->server_ready);

    while (1) {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);

        int new_sock_index = 0;
        for (new_sock_index=0; new_sock_index<max_socks; ++new_sock_index) {
            if (sock[new_sock_index] == INVALID_SOCK) {
                break;
            }
        }

        if (new_sock_index < max_socks) {
            sock[new_sock_index] = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock[new_sock_index] < 0) {
                if (errno == EWOULDBLOCK) { // The listener socket did not accepts any connection
                                            // continue to serve open connections and try to accept again upon the next iteration
                    ESP_LOGV(mthis->TAG, "Bekleyen baglantı yok...");
                } else {
                    log_socket_error(mthis->TAG, listen_sock, errno, "Baglantı kabulünde hata");
                    goto error;
                }
            } else {
                char *ip = (char *) malloc(15);
                mthis->get_clients_address(&source_addr,ip); //ip adresini bul
                char *mac =(char*)malloc(16);
                mthis->cihaz->get_sta_mac(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr,mac); //mac idsini bul
                if (mac!=NULL)
                {
                    device_register_t *Bag=mthis->cihaz->cihazbul(mac); //mac e göre cihazı bul
                    if (Bag!=NULL) //Cihaz varsa ip ve soketi ekle
                        {
                            Bag->socket = sock[new_sock_index];
                            Bag->ip=((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;
                        } else {
                            //Cihaz yoksa oluştur ip ve soketi ekle
                            Bag = mthis->cihaz->cihaz_ekle(mac,TR_UDP);
                            Bag->socket = sock[new_sock_index];
                            Bag->ip=((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;
                        }  
                    ESP_LOGI(mthis->TAG, "[sock=%d]: Connection accepted from IP:%s MAC:%s", sock[new_sock_index], ip,mac);
                    // ...and set the client's socket non-blocking
                    flags = fcntl(sock[new_sock_index], F_GETFL);
                    if (fcntl(sock[new_sock_index], F_SETFL, flags | O_NONBLOCK) == -1) {
                        log_socket_error(mthis->TAG, sock[new_sock_index], errno, "Soket Non-block yapılamadı");
                        goto error;
                    }
                } else {
                    //Cihazın mac id si bbulunamadı
                    ESP_LOGI(mthis->TAG, "[sock=%d]: IP:%s  Mac Id si bulunamadı.. Baglantı kapatılıyor.", sock[new_sock_index],ip);
                    close(sock[new_sock_index]);
                    sock[new_sock_index] = INVALID_SOCK;
                }
                free(mac); mac=NULL;
                free(ip);ip=NULL;                
            }
        }

        for (int i=0; i<max_socks; ++i) {
            if (sock[i] != INVALID_SOCK) {
                int len = try_receive(mthis->TAG, sock[i], rx_buffer, BUFFER_SIZE-1);
                if (len < 0) {
                    close(sock[i]);
                    sock[i] = INVALID_SOCK;
                } else if (len > 0) {
                    //ESP_LOGI(mthis->TAG, "[sock=%d]: Received %.*s", sock[i], len, rx_buffer);
                    if (rx_buffer[len-1]=='&')
                      {
                        device_register_t *Bag=mthis->cihaz->cihazbul_soket(sock[i]);
                        rx_buffer[len-1]=0;
                        if (mthis->callback!=NULL)
                        {
                            char *res = (char *)malloc(512);
                            memset(res,0,512);
                            mthis->callback((char*)rx_buffer,Bag->device_id, TR_UDP,(void *)res, false);
                            if (strlen(res)>2) 
                            {
                                strcat(res,"&");
                                send(sock[i], res, strlen(res), 0);
                                vTaskDelay(50 / portTICK_PERIOD_MS);
                            }
                            free(res);res=NULL;
                        }
                      }
                }

            } // one client's socket
        } // for all sockets

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(YIELD_TO_ALL_MS));
    }
error:
    mthis->server_fatal_error=true;
    if (listen_sock != INVALID_SOCK) {
        close(listen_sock);
    }

    for (int i=0; i<max_socks; ++i) {
        if (sock[i] != INVALID_SOCK) {
            close(sock[i]);
        }
    }
     ESP_LOGE(mthis->TAG, "Server Kapatıldı");
    free(address_info);
    free(rx_buffer);
    rx_buffer = NULL;
    vTaskDelete(NULL);
}
#endif

#ifdef SERVER_MODE
bool TcpServer::Start(home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      uint16_t udpport, 
                      transmisyon_callback_t cb,
                      Cihazlar *chz
                      )
{   
    TcpPort = tcpport;
    UdpPort = udpport;
    sprintf(port,"%d",TcpPort);

    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    sprintf(adres,IPSTR,IP2STR(&ip_info.ip));
    BroadcastAddr = ip_info.ip.addr | (uint32_t)0xFF000000UL;

    net_config = cnf;
    dev_config = hcnf;
    
    callback = cb;
    
        cihaz = chz;
        server_ready = xSemaphoreCreateBinary();
        assert(server_ready);
        xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)this, 5, NULL);
        xSemaphoreTake(server_ready, portMAX_DELAY);
        vSemaphoreDelete(server_ready);
        xTaskCreate(UDP_broadcast_listener, "udp_listener", 4096, (void*)this, 5, NULL);
        return !server_fatal_error;
}
#endif

#ifdef CLIENT_MODE
bool TcpServer::Start(home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      uint16_t udpport, 
                      transmisyon_callback_t cb,
                      const char *srvip
                      )
{   
    TcpPort = tcpport;
    UdpPort = udpport;
    sprintf(port,"%d",TcpPort);

    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    sprintf(adres,IPSTR,IP2STR(&ip_info.ip));
    BroadcastAddr = ip_info.ip.addr | (uint32_t)0xFF000000UL;

    net_config = cnf;
    dev_config = hcnf;
    
    callback = cb;
      strcpy(adres,srvip);
    return !server_fatal_error;
}
#endif 

esp_err_t TcpServer::Send(char *data, uint8_t id)
{
    //Soketi bul
    device_register_t *bag = cihaz->cihazbul(id);
    if (bag!=NULL) {
        char *dt = (char*)malloc(strlen(data)+2);
        strcpy(dt,data);
        strcat(dt,"&");
        int data_size = strlen(dt);
        int len = socket_send(TAG, bag->socket, dt, data_size);
        free(dt);
        if (len!=data_size) return ESP_FAIL;
        return ESP_OK;
    } else return ESP_FAIL;
}


esp_err_t TcpServer::Udp_Send(const char *data)
{
    char *dt = (char *)malloc(strlen(data)+2);
    strcpy((char*)dt,data);
    strcat((char*)dt,"&");
    int datasize = strlen(dt);
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
    if (Udp_Send(dat)==ESP_FAIL) ESP_LOGE(TAG, "Udp Send ERROR");
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
        dest_addr.sin_addr.s_addr = mthis->BroadcastAddr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(UDP_BROADCAST_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        mthis->Broadcast_Soket = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (mthis->Broadcast_Soket < 0) {ESP_LOGE(mthis->TAG, "Udp ERR");break;}

        int broadcast = 1;
        setsockopt(mthis->Broadcast_Soket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        if (bind(mthis->Broadcast_Soket, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) { ESP_LOGE(mthis->TAG, "Udp ERR");break; }
        ESP_LOGI(mthis->TAG, "%d UDP Broadcast Socket Listening", UDP_BROADCAST_PORT); 
        while (1) {
            struct sockaddr_storage source_addr; 
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(mthis->Broadcast_Soket, rx_buffer, 510, 0, (struct sockaddr *)&source_addr, &socklen);
            if (len < 0) { ESP_LOGE(mthis->TAG, "Udp ERR"); break; }
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
                        #ifdef SERVER_MODE
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
                              //mthis->Broadcast_Command("sinfo_ack",254); 
                              ESP_LOGI(mthis->TAG,"%d  server info sended ",iid); 
                            }
                        #endif    
                        cJSON_Delete(rcv);  
                      }

                    printf("UDP broadcast %s\n",rx_buffer); 

                 }
            vTaskDelay(10);
        }
        if (mthis->Broadcast_Soket != -1) {
            ESP_LOGE(mthis->TAG, "Udp ERR");
            shutdown(mthis->Broadcast_Soket, 0);
            close(mthis->Broadcast_Soket);
        }
    }
    free(rx_buffer);
}

#ifdef CLIENT_MODE
void TcpServer::tcp_client_task(void *arg)
{
    static const char *TAG = "nonblocking-socket-client";
    TcpServer *mthis = (TcpServer *)arg;
    static char rx_buffer[128];

    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;
    int sock = INVALID_SOCK;

while (1)
{  
    //Soket oluştur oluşturamazsan server kapat
    int res = getaddrinfo(DEST_ADDR, DEST_PORT, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(TAG, "couldn't get hostname for `%s` "
                      "getaddrinfo() returns %d, addrinfo=%p", DEST_ADDR, res, address_info);
        goto fatalerror;
    }
    // Creating client's socket
    sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (sock < 0) {
        log_socket_error(TAG, sock, errno, "Unable to create socket");
        goto fatalerror;
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%s", DEST_ADDR, DEST_PORT);

    // Marking the socket as non-blocking
    int flags = fcntl(sock, F_GETFL);
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_socket_error(TAG, sock, errno, "Unable to set socket non blocking");
    }

    if (connect(sock, address_info->ai_addr, address_info->ai_addrlen) != 0) {
        if (errno == EINPROGRESS) {
            ESP_LOGD(TAG, "connection in progress");
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            // Connection in progress -> have to wait until the connecting socket is marked as writable, i.e. connection completes
            res = select(sock+1, NULL, &fdset, NULL, NULL);
            if (res < 0) {
                log_socket_error(TAG, sock, errno, "Error during connection: select for socket to be writable");
                goto error;
            } else if (res == 0) {
                log_socket_error(TAG, sock, errno, "Connection timeout: select for socket to be writable");
                goto error;
            } else {
                int sockerr;
                socklen_t len = (socklen_t)sizeof(int);

                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                    log_socket_error(TAG, sock, errno, "Error when getting socket error using getsockopt()");
                    goto error;
                }
                if (sockerr) {
                    log_socket_error(TAG, sock, sockerr, "Connection error");
                    goto error;
                }
            }
        } else {
            log_socket_error(TAG, sock, errno, "Socket is unable to connect");
            goto error;
        }
    }

    while (1)
    {
        //soketten hata alınıncaya kadar bekle
        do {
            len = try_receive(TAG, sock, rx_buffer, sizeof(rx_buffer));
            if (len < 0) {
                ESP_LOGE(TAG, "Error occurred during try_receive");
                goto error;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        } while (len == 0);
      ESP_LOGI(TAG, "Received: %.*s", len, rx_buffer);
    }

error:
    if (sock != INVALID_SOCK) {
        ESP_LOGE(TAG, "Socket closed");
        close(sock);
    }
}

fatalerror:
    free(address_info);
    ESP_LOGE(TAG, "Client Closed");
    vTaskDelete(NULL);

}
#endif

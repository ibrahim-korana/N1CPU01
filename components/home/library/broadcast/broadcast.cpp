#include "broadcast.h"

void UdpBroadcast::Command(const char *cm, uint8_t dev, uint8_t function_count)
{
    char * mm = (char *) malloc(20);
    
    sprintf(mm,"%02X:%02X:%02X:%02X:%02X:%02X",
        Net_Config->mac[0],
        Net_Config->mac[1],
        Net_Config->mac[2],
        Net_Config->mac[3],
        Net_Config->mac[4],
        Net_Config->mac[5]
    );
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", cm);
    cJSON_AddNumberToObject(root, "id", dev);  
    cJSON_AddStringToObject(root, "mac", mm);
    cJSON_AddStringToObject(root, "ip",Adr.to_string(Net_Config->home_ip));
    cJSON_AddNumberToObject(root, "blok",Glo_Config->blok); 
    cJSON_AddNumberToObject(root, "daire",Glo_Config->daire); 

    char *dat = cJSON_PrintUnformatted(root);
    if (Send(dat)==ESP_FAIL) ESP_LOGE(TAG, "Udp Send ERROR");
    free(mm);
    cJSON_free(dat);
    cJSON_Delete(root); 
}

bool UdpBroadcast::Start(
               home_network_config_t *netconfig, 
               home_global_config_t *globalconfig, 
               uint16_t sport,
               rs485_callback_t cb
               )
{   
    port= sport;
    Net_Config = netconfig;
    Glo_Config = globalconfig;
    callback = cb;

    IPAddr adr = IPAddr();
    //subnetBroadcast = ip_info.ip.addr | ~(ip_info.netmask.addr); 
    //printf("ADRES %s\n",adr.to_string(Net_Config->home_broadcast));
    xTaskCreate(UDP_broadcast_listener, "udp_listener", 4096, (void*)this, 5, NULL);
    return !server_fatal_error;
}

esp_err_t UdpBroadcast::Send(const char *data)
{
    char *dt = (char *)malloc(strlen(data)+2);
    strcpy((char*)dt,data);
    strcat((char*)dt,"&");
    int datasize = strlen(dt);
    
    static int iobAnnounce;
    const char broadcastEnable = 1;
    struct sockaddr_in servaddrAnnounce;
    iobAnnounce = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(iobAnnounce, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    memset(&servaddrAnnounce, 0, sizeof(servaddrAnnounce));
    servaddrAnnounce.sin_family = AF_INET;
    servaddrAnnounce.sin_port = htons(port);
    servaddrAnnounce.sin_addr.s_addr = Net_Config->home_broadcast;
    int sz=sendto(iobAnnounce,dt,datasize,0,(const struct sockaddr *) &servaddrAnnounce,sizeof(servaddrAnnounce));

    ESP_LOGI(TAG,"GIDEN >> Broadcast: %s %s", (sz==datasize)?"OK":"FAIL",data);

    free(dt); 
    dt=NULL;
    close(iobAnnounce);    
    if (sz!=datasize) {return ESP_FAIL;}
  return ESP_OK;   
}

void UdpBroadcast::UDP_broadcast_listener(void *arg)
{
    char *rx_buffer = (char *)malloc(BUFFER_SIZE);
    int addr_family = 0;
    int ip_protocol = 0;
    int broadcast = 1;
    UdpBroadcast *mthis = (UdpBroadcast *)arg;
    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = mthis->Net_Config->home_broadcast;

        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(mthis->port);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        mthis->Broadcast_Soket = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (mthis->Broadcast_Soket < 0) {ESP_LOGE(mthis->TAG, "Udp Soket Hatası");goto fatalerror;}
        setsockopt(mthis->Broadcast_Soket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        struct timeval read_timeout;
        read_timeout.tv_sec = 0;
        read_timeout.tv_usec = 10;
        setsockopt(mthis->Broadcast_Soket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

        
        if (bind(mthis->Broadcast_Soket, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) { ESP_LOGE(mthis->TAG, "Socket not binding");goto fatalerror; }
        ESP_LOGI(mthis->TAG, "%s:%d UDP Broadcast Socket Listening", mthis->Adr.to_string(mthis->Net_Config->home_broadcast), mthis->port); 
        while (1) {
            struct sockaddr_storage source_addr; 
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(mthis->Broadcast_Soket, rx_buffer, BUFFER_SIZE-1, 0, (struct sockaddr *)&source_addr, &socklen);
            if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            //len= 0;   // Not an error
            }

            if (len < 0) { ESP_LOGE(mthis->TAG, "Soket Okuma Hatası"); goto fatalerror; }
            // Data received
            else if (len>0){
                    rx_buffer[len] = 0; 
                    
                    if (rx_buffer[len-1]=='&') 
                      {
                        rx_buffer[len-1] = 0;
                        if (mthis->callback!=NULL)
                          mthis->callback(rx_buffer,255,TR_UDP);
                        //char addr_str[128]; 
                        //inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                        //uint32_t ip = (uint32_t)(((struct sockaddr_in *)&source_addr)->sin_addr).s_addr;
                        /* 
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
                        */
                        cJSON *rcv = cJSON_Parse((char*)rx_buffer);
                        char *command = (char *)calloc(1,20);
                        JSON_getstring(rcv,"com", command);

                        if (strcmp(command,"sinfo")==0) { 
                              mthis->Command("sinfo_ack",254,0); 
                              ESP_LOGI(mthis->TAG,"server info sended "); 
                            }
                        free(command);
                        cJSON_Delete(rcv);  
                      
                      }

                    //printf("UDP broadcast %s\n",rx_buffer); 

                 }
           // vTaskDelay(10);
            if (mthis->wait) break;
        }

        fatalerror:
        if (mthis->Broadcast_Soket != -1) {
            ESP_LOGE(mthis->TAG, "Server kapatılıyor");
            //shutdown(mthis->Broadcast_Soket, 0);
            close(mthis->Broadcast_Soket);
            mthis->Broadcast_Soket = -1;
        }
        while(mthis->wait) vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    free(rx_buffer);
}

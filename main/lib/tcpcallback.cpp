uint8_t ahex2int(char a, char b){

    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}
//-----------------------------------------
bool send_action(cJSON *rcv)
{
    char *dat = cJSON_PrintUnformatted(rcv);
            while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
            uart.Sender(dat,RS485_MASTER);
            vTaskDelay(50/portTICK_PERIOD_MS); 
    cJSON_free(dat);
    return true;
}
//-----------------------------------------
void command_transfer(cJSON *rcv_json, sender_t snd)
{
    char *command = (char *)calloc(1,15);

    JSON_getstring(rcv_json,"com", command); 

    ESP_LOGI("TCP_CALLBACK command_transfer","COMMAND : %s",command);

    if (strncmp(command,"COM",3)==0) send_action(rcv_json);

    if (strcmp(command,"start_senaryo")==0) {
        uint8_t *aa = (uint8_t *) malloc(1);
        *aa = 1;
        start_sen(aa);
        free(aa);
    }

    if (strcmp(command,"stop_senaryo")==0) {
        uint8_t *aa = (uint8_t *) malloc(1);
        *aa = 1;
        stop_sen(aa);
        free(aa);
    }

    if (strcmp(command,"intro")==0) {
        sender_type = snd;
        send_action(rcv_json);

    }
    if (strcmp(command,"sintro")==0) {
        //Cihaz Alt özelliklerini iste
        sender_type = snd;
        send_action(rcv_json);
    }
    if (strcmp(command,"status")==0) {
        //tüm cihazların veya 1 cihazın durum/larını iste
        sender_type = snd;
        send_action(rcv_json);
    }
    if (strcmp(command,"event")==0) {
        //mqttden gelen mesajı cpu2 ye aktararak fonksiyonların
        //durum degiştirmesini sağlar
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"sevent")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"spevent")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"loc")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }

    if (strcmp(command,"all")==0) {
        sender_type = snd;
        send_action(rcv_json);
    }

    if (strcmp(command,"sen")==0) {
        uint8_t snn = 0;
        bool change = false;
        if (JSON_getint(rcv_json,"no", &snn)) {
            GlobalConfig.aktif_senaryo=snn;
            change=true;
        }
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "sen");
        cJSON_AddNumberToObject(root, "no", GlobalConfig.aktif_senaryo);
        char *dat = cJSON_PrintUnformatted(root);
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(dat);
        mqtt.publish(dat,strlen(dat));
        cJSON_free(dat);
        cJSON_Delete(root);
        if (change)
          {
            disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
            ESP_LOGI(TAG,"Senaryo No : %d",GlobalConfig.aktif_senaryo);
          }
    }
    free(command);
    
}


void tcp_callback(char *data, uint8_t sender, transmisyon_t transmisyon, void *response, bool responsevar)
{
    
    cJSON *rcv_json = cJSON_Parse(data);
    char *command = (char *)calloc(1,15);
    bool trn = true;

    JSON_getstring(rcv_json,"com", command); 

    if (strcmp(command,"ping")!=0) ESP_LOGI(TAG,"TCP << %d %s",sender, data);

    if (strcmp(command,"set")==0)
     {
        char *mc = (char *)malloc(20);
        char *se = (char *)malloc(30);
        char *mq = (char *)malloc(30);

        uint8_t bl;
        uint8_t da;
        
        JSON_getstring(rcv_json,"mac", mc);
        JSON_getstring(rcv_json,"seri", se);
        JSON_getstring(rcv_json,"mqtt", mq);

        if (JSON_getint(rcv_json,"blok",&bl)) GlobalConfig.blok = bl;
        if (JSON_getint(rcv_json,"daire",&da)) GlobalConfig.daire = da;

        NetworkConfig.mac[0]=ahex2int(mc[0],mc[1]);
        NetworkConfig.mac[1]=ahex2int(mc[2],mc[3]);
        NetworkConfig.mac[2]=ahex2int(mc[4],mc[5]);
        NetworkConfig.mac[3]=ahex2int(mc[6],mc[7]);
        NetworkConfig.mac[4]=ahex2int(mc[8],mc[9]);
        NetworkConfig.mac[5]=ahex2int(mc[10],mc[11]);
        strcpy((char*)GlobalConfig.license,se);
        strcpy((char*)GlobalConfig.mqtt_server,mq);
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
        disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0); 
        free(mc);
        free(se);
        free(mq);
        esp_restart();
     }
    if (strcmp(command,"ping")==0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "pong");
        char *dat = cJSON_PrintUnformatted(root);
        strcpy((char*)response,dat);
        cJSON_free(dat);
        cJSON_Delete(root);
        trn = false;
     }
    if (strcmp(command,"T_REQ")==0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "T_ACK");
        unsigned long abc = rtc.getLocalEpoch();
        cJSON_AddNumberToObject(root, "time", abc);
        char *dat = cJSON_PrintUnformatted(root);
        strcpy((char*)response,dat);
        cJSON_free(dat);
        cJSON_Delete(root);
        trn = false;
    } 

    if (trn) command_transfer(rcv_json, SEND_TCP);
    free(command);
    cJSON_Delete(rcv_json);
    
}

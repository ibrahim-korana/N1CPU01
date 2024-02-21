


void config(void)
{
    ESP_LOGI(TAG, "INITIALIZING...");
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<LLED0) | (1ULL<<CPU2_RESET) | (1ULL<<LLED1) | (1ULL<<LLED2 );
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<DEF_RESET_BUTTON);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)LLED0, 0);
    gpio_set_level((gpio_num_t)LLED1, 0);
    gpio_set_level((gpio_num_t)LLED2, 0);
    gpio_set_level((gpio_num_t)CPU2_RESET, 0);
    

    pcf8563_init_desc(&i2cbus,(i2c_port_t)0,(gpio_num_t)21, (gpio_num_t)22);

    //ekran = OLEDDisplay_init(0,0x78,21,22);
    ekran = OLEDDisplay_init(&i2cbus,0x78);
    OLEDDisplay_clear(ekran);
    OLEDDisplay_flipScreenVertically(ekran);
    OLEDDisplay_setFont(ekran,ArialMT_Plain_10);
    line = (line_t *) malloc(sizeof(line_t));
    for (int i=0;i<4;i++)
      {
        line->satir[i] = (char *)malloc(32);
        memset(line->satir[i],0,32);
      }
    line->ekran = ekran;  
    line->pb = 0;
    line->error = false;
    line->update= true;
    line->off = false;  
    line->screen_minute = 0;
    line->error_flag = 0;
    line->kapali = false;
       
    strcpy(line->satir[0],"Embeded Version");
    strcpy(line->satir[1],desc->version); 
    strcpy(line->satir[2],"");
    sceen_push();
    vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
        
    logo_print(ekran);

    if(esp_event_loop_create_default()!=ESP_OK) {ESP_LOGE(TAG,"esp_event_loop_create_default ERROR "); }
	  ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, all_event, NULL, NULL));
    OLEDDisplay_drawProgressBar(ekran,14,25,100,5,10);
    
    strcpy(line->satir[0], "Initializing");  

    get_sha256_of_partitions();

    //---------------- Disk Init ---------------------------
    line->pb = 10;
    strcpy(line->satir[1],"01 Disk");
    sceen_push();
    esp_err_t err = disk.init();
    if (!err)
      {
        line->error=true;
        strcpy(line->satir[3],"01 Disk ERROR");
        sceen_push();
        vTaskDelay(2000/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(!err);
      }
    strcpy(line->satir[1],"01 Disk OK");
    sceen_push();
    vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
    //------------------------------------------------------
    //---------------- RTC Init ---------------------------
      
    
      line->pb = 20;
      strcpy(line->satir[1],"02 Saat");
      sceen_push();
      esp_err_t saaterr = ESP_OK;
      saaterr = pcf8563_init_desc(&i2cbus,(i2c_port_t)0,(gpio_num_t)21, (gpio_num_t)22);
      if(saaterr!=ESP_OK) {
              ESP_LOGE(TAG, "RealTime Clock ERROR !..."); 
              strcpy(line->satir[1],"02 Saat ERROR");
              sceen_push();      
                          } else {
                            char *rr = (char *)malloc(50);
                            struct tm zaman = {};                                       
                            pcf8563_get_time(&i2cbus, &zaman, &valid_time);
                            if (!valid_time) {
                                ESP_LOGE(TAG, "Saati Ayarlaniyor");
                                struct tm tt0 = {};
                                struct tm tt1 = {} ;
                                bool val = false;
                                tt0.tm_sec = 0;
                                tt0.tm_min = 0;
                                tt0.tm_hour = 0;
                                tt0.tm_mday = 28;
                                tt0.tm_mon = 6;
                                tt0.tm_year = 123;
                                pcf8563_set_time(&i2cbus,&tt0);
                                pcf8563_get_time(&i2cbus,&tt1,&val);                                
                                strftime(rr, 50, "%02d.%m.%Y %H:%M", &tt1);
                                ESP_LOGI(TAG, "RTC (Set) %s",rr);                               
                            } else {
                                strftime(rr, 50, "%02d.%m.%Y %H:%M", &zaman); 
                                time_t tt = mktime(&zaman);
                                Set_ctime(tt);
                                ESP_LOGI(TAG,"RTC (Not Set) %s",rr);
                            }
                            strcpy(line->satir[1],"02 Saat OK");
                            strcpy(line->satir[2],rr);
                            sceen_push();
                            free(rr);
                                }
    vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);     
                    
    //-------------------------------------
    disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig), 0);
    if (NetworkConfig.home_default==0 ) {
        //Network ayarları diskte kayıtlı değil. Kaydet.
         network_default_config();
         disk.file_control(NETWORK_FILE);
         disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
         if (NetworkConfig.home_default==0 ) ESP_LOGW(TAG, "Network Initilalize File ERROR !...");
    }

    disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig), 0);
    if (GlobalConfig.home_default==0 ) {
         global_default_config();
         disk.file_control(GLOBAL_FILE);
         disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
         if (GlobalConfig.home_default==0 ) ESP_LOGW(TAG,"\n\nGlobal Initilalize File ERROR !...\n\n");
    }

    disk.list("/config", "*.*");

    //----------- NETWORK INIT ------------------------------
        line->pb = 30;
        strcpy(line->satir[1],"03 Network");
        sceen_push();
        bool netstatus = false;
        if (NetworkConfig.wifi_type==HOME_ETHERNET)
        {           
            netstatus = Eth.start(NetworkConfig, &GlobalConfig);
            if (GlobalConfig.random_mac)
              {
                  GlobalConfig.random_mac = 0;
                  disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
              }
            if (netstatus!=ESP_OK) { 
                led_events_data_t ld={};
                ld.state = 1;
                ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
                ESP_LOGE(TAG, "ETHERNET Init ERROR !..."); 
                strcpy(line->satir[1],"Cihaz Resetlenecek");
                sceen_push();
                vTaskDelay(5000/portTICK_PERIOD_MS);
                ESP_ERROR_CHECK(netstatus);
            } else {
                strcpy(line->satir[1],"03 Ethernet OK");
                sceen_push();
            }
        }

        if (NetworkConfig.wifi_type==HOME_WIFI_AP || NetworkConfig.wifi_type==HOME_WIFI_STA)
        { 
            wifi.init(&NetworkConfig);
            if (NetworkConfig.wifi_type==HOME_WIFI_AP) netstatus = wifi.Ap_Start();
            if (NetworkConfig.wifi_type==HOME_WIFI_STA) netstatus = wifi.Station_Start();
            if (netstatus!=ESP_OK) { 
                ESP_LOGE(TAG, "Wifi Init ERROR !..."); 
                strcpy(line->satir[1],"03 Wifi ERROR");
                sceen_push();
                vTaskDelay(2000/portTICK_PERIOD_MS);
                ESP_ERROR_CHECK(!netstatus);
            } else {
                if (NetworkConfig.wifi_type==HOME_WIFI_AP) strcpy(line->satir[1],"03 Wifi Ap OK");
                if (NetworkConfig.wifi_type==HOME_WIFI_STA) strcpy(line->satir[1],"03 Wifi Sta OK");
                sceen_push();
            }
        }

    vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
    //--------------------------------------------------   
    //------ UART INIT ---------------------------------
    
    line->pb = 40;
    strcpy(line->satir[1],"04 CPU2");
    sceen_push();
    uart_cfg.uart_num = 2;
    uart_cfg.dev_num  = DEVICE_ID;
    uart_cfg.rx_pin   = 27;
    uart_cfg.tx_pin   = 26;
    uart_cfg.atx_pin  = 32;
    uart_cfg.arx_pin  = 25;
    uart_cfg.int_pin  = 33;

    //int 32 de
    uart_cfg.baud   = 921600;

    uart.initialize(&uart_cfg, &uart_callback);
    uart.pong_start(254,&CPU2_Ping_Reset,LLED0);
    
    strcpy(line->satir[1],"04 CPU2 OK");
    sceen_push();
    vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
    //--------------------------------------------------- 

    if (netstatus==ESP_OK)
     {    
        line->pb = 50;
        strcpy(line->satir[1],"05 Sntp");
        sceen_push();
        ESP_LOGI(TAG, "Initializing SNTP"); 
        sntp_set_time_sync_notification_cb(time_sync);
        if (GlobalConfig.set_sntp==1)
        {
          Set_SystemTime_SNTP_diff();
          GlobalConfig.set_sntp = 0;
          disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
        } else {Set_SystemTime_SNTP();}
        strcpy(line->satir[1],"05 Sntp OK");
        sceen_push();
        vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);

        line->pb = 60;
        strcpy(line->satir[1],"06 Tcp Server");
        sceen_push();
        if (GlobalConfig.tcp_start==1)
          {
            tcpserver.start(TCP_PORT, &tcp_callback);
            broadcast.Start(&NetworkConfig,&GlobalConfig,5718, NULL);
            strcpy(line->satir[1],"06 Tcp Server OK");
            sceen_push();
            vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
          } else {
            strcpy(line->satir[1],"06 Tcp Server CLOSED");
            sceen_push();
            vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
          }  
        
        line->pb = 70;
        strcpy(line->satir[1],"07 Web Server");
        sceen_push();
        ESP_LOGI(TAG, "WEB START");
        if (GlobalConfig.http_start==1)
        {
          ESP_ERROR_CHECK(start_rest_server("/config",&NetworkConfig, &GlobalConfig, &webwrite, &defreset));
          strcpy(line->satir[1],"07 Web Server OK");
          sceen_push();
          vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
        } else {
            strcpy(line->satir[1],"07 Web Server CLOSED");
            sceen_push();
            vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);
        }  
        
        line->pb = 80;
        strcpy(line->satir[1],"08 Mqtt");
        sceen_push();
        ESP_LOGI(TAG, "MQTT START");        
        mqtt.init(GlobalConfig, &mqtt_callback, &self_reset); 
        mqtt.start();    
        strcpy(line->satir[1],"08 Mqtt OK");
        sceen_push();
        vTaskDelay(FASTBOOT_DELAY/portTICK_PERIOD_MS);   

        if (netconfig->upgrade==1)
          {
             strcpy(line->satir[1],"09 Firmware Upgrade");
             sceen_push();
             xTaskCreate(&ota_task, "ota_task", 8192, &NetworkConfig, 5, NULL);
          }        
     }

    line->pb = 0;
    sprintf(line->satir[0],"IP : %s",(char*)NetworkConfig.ip);
    struct tm tt0;
    time_read(&tt0);
    char *rr = (char *)malloc(50);
    memset((char*)rr,0,48);
    strftime(rr, 50, "%d.%m.%Y %H:%M", &tt0);
    strcpy(line->satir[1],rr);

    sprintf(line->satir[2],"MAC : %02X:%02X:%02X:%02X:%02X:%02X",
        NetworkConfig.mac[0],
        NetworkConfig.mac[1],
        NetworkConfig.mac[2],
        NetworkConfig.mac[3],
        NetworkConfig.mac[4],
        NetworkConfig.mac[5]
    );
    sceen_push();

    char *NetType = (char *)malloc(50);
    transmisyon_type_convert(NetworkConfig.wifi_type,NetType);
    strcpy((char*)NetworkConfig.version,(char*)desc->version);

    ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG,"         |        ICE MAIN BOX        |");
    ESP_LOGI(TAG,"         |            CPU 1           |");
    ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG,"               License : %s", GlobalConfig.license); 
    ESP_LOGI(TAG,"           Mqtt Server : %s%s", "mqtt://",GlobalConfig.mqtt_server); 
    ESP_LOGI(TAG,"        Uart Device ID : %d", DEVICE_ID);
    ESP_LOGI(TAG,"                  Blok : %d", GlobalConfig.blok);
    ESP_LOGI(TAG,"                 Daire : %d", GlobalConfig.daire);
    ESP_LOGI(TAG,"             Cihaz ADI : %s", GlobalConfig.device_name); 
    ESP_LOGI(TAG,"            Tcp Server : %d", GlobalConfig.tcp_start);
    ESP_LOGI(TAG,"            Web Server : %d", GlobalConfig.http_start);
    ESP_LOGI(TAG,"         Reset Servisi : %d", GlobalConfig.reset_servisi);

    ESP_LOGI(TAG,"               Network : %s", (netstatus==ESP_OK)?"OK":"Network YOK");
    ESP_LOGI(TAG,"           Transmisyon : %s", NetType);
    ESP_LOGI(TAG,"                    Ip : %s" ,Adr.to_string(NetworkConfig.home_ip));
    ESP_LOGI(TAG,"               Netmask : %s" ,Adr.to_string(NetworkConfig.home_netmask));
    ESP_LOGI(TAG,"               Gateway : %s" ,Adr.to_string(NetworkConfig.home_gateway));
    ESP_LOGI(TAG,"             Broadcast : %s" ,Adr.to_string(NetworkConfig.home_broadcast));
    ESP_LOGI(TAG,"            Dns Server : %s" ,NetworkConfig.dns);
    ESP_LOGI(TAG,"     Backup Dns Server : %s" ,NetworkConfig.backup_dns);

    ESP_LOGI(TAG,"                   MAC : %s" ,Adr.mac_to_string(NetworkConfig.mac));
    ESP_LOGI(TAG,"              Tcp Port : %d" ,TCP_PORT);
    ESP_LOGI(TAG,"             Date/Time : %s" ,rr);
    ESP_LOGI(TAG,"           App Version : %s" ,desc->version);
    ESP_LOGI(TAG,"              App Name : %s" ,desc->project_name);
    ESP_LOGI(TAG,"             Free heap : %lu" ,esp_get_free_heap_size());

    free(rr);
    free(NetType);   
    init_status=false; 

}

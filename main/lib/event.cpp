
#include "esp_log.h"
#include "esp_http_server.h"


const char* get_id_string(esp_event_base_t base, int32_t id) 
{
    const char* event = "TASK_ITERATION_EVENT";
/*
    if (base==ESP_HTTP_SERVER_EVENT)
    {
      if (id==HTTP_SERVER_EVENT_ERROR ) event = "HTTP_SERVER_EVENT_ERROR";
      if (id==HTTP_SERVER_EVENT_START ) event = "HTTP_SERVER_EVENT_START";
      if (id==HTTP_SERVER_EVENT_ON_CONNECTED  ) event = "HTTP_SERVER_EVENT_ON_CONNECTED";
      if (id==HTTP_SERVER_EVENT_ON_HEADER ) event = "HTTP_SERVER_EVENT_ON_HEADER";
      if (id==HTTP_SERVER_EVENT_HEADERS_SENT ) event = "HTTP_SERVER_EVENT_HEADERS_SENT";
      if (id==HTTP_SERVER_EVENT_ON_DATA ) event = "HTTP_SERVER_EVENT_ON_DATA";
      if (id==HTTP_SERVER_EVENT_SENT_DATA ) event = "HTTP_SERVER_EVENT_SENT_DATA";
      if (id==HTTP_SERVER_EVENT_DISCONNECTED ) event = "HTTP_SERVER_EVENT_DISCONNECTED";
      if (id==HTTP_SERVER_EVENT_STOP ) event = "HTTP_SERVER_EVENT_STOP";
    }
*/
    if (base==IP_EVENT)
      {
        if (id==IP_EVENT_STA_GOT_IP ) event = "IP_EVENT_STA_GOT_IP";
        if (id==IP_EVENT_STA_LOST_IP  ) event = "IP_EVENT_STA_LOST_IP ";
        if (id==IP_EVENT_AP_STAIPASSIGNED  ) event = "IP_EVENT_AP_STAIPASSIGNED ";
        if (id==IP_EVENT_GOT_IP6) event = "IP_EVENT_GOT_IP6";
        if (id==IP_EVENT_ETH_GOT_IP ) event = "IP_EVENT_ETH_GOT_IP";
        if (id==IP_EVENT_ETH_LOST_IP ) event = "IP_EVENT_ETH_LOST_IP";

      }
      
    if (base==WIFI_EVENT)
      {
        if (id==WIFI_EVENT_WIFI_READY) event = "WIFI_EVENT_WIFI_READY";
        if (id==WIFI_EVENT_SCAN_DONE) event = "WIFI_EVENT_SCAN_DONE";
        if (id==WIFI_EVENT_STA_START) event = "WIFI_EVENT_STA_START";
        if (id==WIFI_EVENT_STA_STOP ) event = "WIFI_EVENT_STA_STOP ";
        if (id==WIFI_EVENT_STA_CONNECTED ) event = "WIFI_EVENT_STA_CONNECTED ";
        if (id==WIFI_EVENT_STA_DISCONNECTED ) event = "WIFI_EVENT_STA_DISCONNECTED ";
        if (id==WIFI_EVENT_STA_AUTHMODE_CHANGE ) event = "WIFI_EVENT_STA_AUTHMODE_CHANGE ";
        if (id==WIFI_EVENT_AP_START ) event = "WIFI_EVENT_AP_START ";
        if (id==WIFI_EVENT_AP_STOP ) event = "WIFI_EVENT_AP_STOP ";
        if (id==WIFI_EVENT_AP_STACONNECTED ) event = "WIFI_EVENT_AP_STACONNECTED ";
        if (id==WIFI_EVENT_AP_STADISCONNECTED  ) event = "WIFI_EVENT_AP_STADISCONNECTED  ";
        if (id==WIFI_EVENT_AP_PROBEREQRECVED  ) event = "WIFI_EVENT_AP_PROBEREQRECVED  ";
      }
      
    if (base==ETH_EVENT)
      {
    	if (id==ETHERNET_EVENT_START) event = "ETHERNET_EVENT_START";
    	if (id==ETHERNET_EVENT_STOP ) event = "ETHERNET_EVENT_STOP ";
    	if (id==ETHERNET_EVENT_CONNECTED  ) event = "ETHERNET_EVENT_CONNECTED";
    	if (id==ETHERNET_EVENT_DISCONNECTED  ) event = "ETHERNET_EVENT_DISCONNECTED";
      }

    if (base==LED_EVENTS)
    {
    	if (id==LED_EVENTS_ETH) event = "LED_ETHERNET";
    	if (id==LED_EVENTS_MQTT) event = "LED_MQTT";
      if (id==LED_EVENTS_PING) event = "PING";
    }

    return event;
}
void all_event(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if (base!=LED_EVENTS) ESP_LOGW(TAG, "%lu %s:%s", id , base, get_id_string(base, id));
    //if (GlobalConfig.comminication==TR_UDP)
    {
        if (base==IP_EVENT)
          {
            if (id==IP_EVENT_STA_GOT_IP || id==IP_EVENT_ETH_GOT_IP)
              {
                // Net.wifi_update_clients();
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                NetworkConfig.home_ip = event->ip_info.ip.addr;
                NetworkConfig.home_netmask = event->ip_info.netmask.addr;
                NetworkConfig.home_gateway = event->ip_info.gw.addr;
                NetworkConfig.home_broadcast = (uint32_t)(NetworkConfig.home_ip) | (uint32_t)0xFF000000UL;

                strcpy((char *)NetworkConfig.ip,Adr.to_string(NetworkConfig.home_ip));
                strcpy((char *)NetworkConfig.netmask,Adr.to_string(NetworkConfig.home_netmask));
                strcpy((char *)NetworkConfig.gateway,Adr.to_string(NetworkConfig.home_gateway));
                //tcpclient.wait = false;
                if (id==IP_EVENT_ETH_GOT_IP) {
                  ESP_LOGI(TAG, "Ethernet GOT IP %s", NetworkConfig.ip);
                  strcpy(line->satir[1],"Ethernet Got IP");
                  sceen_push(); 
                  Eth.set_connect_bit();
                  
                }
                if (id==IP_EVENT_STA_GOT_IP) wifi.set_connection_bit();
              }
          }
          
        
        if (base==WIFI_EVENT)
          {
            if (id==WIFI_EVENT_STA_DISCONNECTED)
              {
                 // tcpclient.wait = true;
                  if (wifi.retry < WIFI_MAXIMUM_RETRY) {
                	  wifi.Station_connect();
                      wifi.retry++;
                      ESP_LOGW(TAG, "Tekrar Baglanıyor %d",WIFI_MAXIMUM_RETRY-wifi.retry);
                                                      } else {
                      ESP_LOGE(TAG,"Wifi Başlatılamadı..");
                      ESP_ERROR_CHECK(ESP_FAIL);
                                                             }
              }
            
            if (id==WIFI_EVENT_STA_START)
              {
                wifi.retry=0;
                ESP_LOGW(TAG, "Wifi Network Connecting..");
                wifi.Station_connect();
              }
            
          }
        
        if (base==ETH_EVENT)
        {
          if (id==ETHERNET_EVENT_START)
          {
            strcpy(line->satir[1],"Ethernet Cable Control");
            sceen_push(); 
          }
        	if (id==ETHERNET_EVENT_CONNECTED)
        	{
        		esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
        		esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, NetworkConfig.mac);
        		ESP_LOGI(TAG, "Ethernet Connected");
            eth_speed_t speed;
            esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
            ESP_LOGI(TAG, "Ethernet Speed:%s", (speed)?"100Mb":"10Mb");

            strcpy(line->satir[1],"Ethernet Cable Connect");
            sceen_push(); 
            //char *xx = (char *)malloc(16);
            //ssd1306_clear_line(&ekran,2,false);
            //strcpy(xx,"Eth Link UP");
            //ssd1306_display_text(&ekran, 2, xx, strlen(xx), false,6);
            //strcpy(xx,"ip waiting");
            //ssd1306_display_text(&ekran, 3, xx, strlen(xx), false,6);

            //free(xx);
        	}
        	if(id==ETHERNET_EVENT_DISCONNECTED)
        	{
        		ESP_LOGI(TAG, "Ethernet Link Down");
        		Eth.set_fail_bit();
        	}
        }

        if (base==LED_EVENTS)
        {
        	led_events_data_t* event = (led_events_data_t*) event_data;
        	if (id==LED_EVENTS_ETH) {
                                     gpio_set_level((gpio_num_t)LLED2, event->state);
                                     if (event->state==1) 
                                       {
                                        line->error_flag = line->error_flag | 0x01;
                                       } else {
                                        line->error_flag = line->error_flag & 0xFE;
                                       }
                                  }
        	if (id==LED_EVENTS_MQTT) {
                      gpio_set_level((gpio_num_t)LLED1, event->state);
                      if (event->state==1) 
                                       {
                                        line->error_flag = line->error_flag | 0x02;
                                       } else {
                                        line->error_flag = line->error_flag & 0xFD;
                                       }
                                   }
          if (id==LED_EVENTS_PING) {
              if (event->state==1) 
                {
                  if ((line->error_flag&0x04)==0x04) {
                    //Daha önceden set edilmiş Ethı resetle
                    ping_stop();
                    Eth.reset();
                    GlobalConfig.reset_counter = GlobalConfig.reset_counter + 1;
                    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
                    esp_restart();
                  }
                  line->error_flag = line->error_flag | 0x04;
                } else {
                  line->error_flag = line->error_flag & 0xFB;
                  ping_start();
                }
          }                         
          if (line->error_flag==0) {
            line->error=false;
            sceen_push();
          }
          if (line->error_flag>0) {
            line->error=true;
            if ((line->error_flag&0x02)==0x02) strcpy(line->satir[3],"Mqtt ERROR");
            if ((line->error_flag&0x04)==0x04) strcpy(line->satir[3],"Ping ERROR");
            if ((line->error_flag&0x01)==0x01) strcpy(line->satir[3],"Ethernet ERROR");
            sceen_push();
          }

        }

        
    }
}




#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_ota_ops.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <sys/socket.h>

#include "assert.h"
#include "esp_system.h"
#include "esp_event.h"

#include "pcf8563.h"
#include "ice_ssd1306.h"


#include "core.h"
#include "jsontool.h"
#include "storage.h"
//#include "eth_encj60.h"
#include "ethernet.h"
#include "ww5500.h"
#include "Network.h"
#include "IpTool.h"
#include "uart.h"
#include "rs485.h"
#include "htime.h"
#include "ESP32Time.h"
#include "tcpserver.h"
#include "broadcast.h"
#include "mqtt.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"


#include "lib/tool/tool.h"
#include "lib/senaryo/senaryo.h"
#include "ping.h"

static const char *TAG = "ANAKUTU_CPU1";
#define GLOBAL_FILE "/config/global.bin"
#define NETWORK_FILE "/config/network.bin"
bool init_status = true;

#define HASH_LEN 32

ESP_EVENT_DEFINE_BASE(LED_EVENTS);
// ESP_EVENT_DECLARE_BASE(EVENT_BASE)
//#define ATMEGA_CONTROL

#define TCP_PORT 5717
#define LLED0 0
#define LLED1 12
#define LLED2 14
#define CPU2_RESET 23
#define DEF_RESET_BUTTON 34

#define MASTER_ID 254
#define DEVICE_ID 253

#define SDA_GPIO 21
#define SCL_GPIO 22
#define OLED_RESET -1

#define FASTBOOT_DELAY 500
#define SLOWBOOT_DELAY 1000

#define SCREEN_OFF_TIME 35

typedef enum {
  SEND_ALL = 0,
  SEND_MQTT = 1,
  SEND_TCP = 2,
} sender_t;


void day_change(struct tm *tminfo);
void min_change(struct tm *tminfo);
void get_sha256_of_partitions(void);

/*
        TANIMLAR
*/
//-----------------------------------------------------------------
static Storage disk = Storage();
i2c_dev_t i2cbus;
bool valid_time = false; 
OLEDDisplay_t *ekran;
const esp_app_desc_t *desc = esp_app_get_description();
home_network_config_t NetworkConfig = {};
home_global_config_t GlobalConfig = {};
//Ethj60 Eth = Ethj60();
Ethernet Eth = Ethernet();
//WW5500 Eth = WW5500();
Network wifi = Network();
IPAddr Adr = IPAddr();
UART_config_t uart_cfg={};
UART uart = UART();
TcpServer tcpserver = TcpServer();
UdpBroadcast broadcast = UdpBroadcast();
Mqtt mqtt = Mqtt();
TimeTrigger trig = TimeTrigger(day_change, min_change);
uint8_t cpu2_reset_counter = 0;
ESP32Time rtc(0);

typedef struct {
    OLEDDisplay_t *ekran;
    char *satir[4];
    bool error;
    bool satir3;
    int pb;
    bool update;
    uint8_t screen_minute;
    bool off;
    uint8_t error_flag;
    bool kapali;
} line_t;

line_t *line; 

sender_t sender_type=SEND_ALL;

void sceen_push()
{
    OLEDDisplay_clear(line->ekran);
    if (line->error==false)
    {
      if(line->kapali==false)
      {
        OLEDDisplay_setColor(line->ekran,WHITE);
        OLEDDisplay_drawString(line->ekran,0, 0, line->satir[0]);
        OLEDDisplay_drawString(line->ekran,0, 11, line->satir[1]);
        if (line->pb>0) {
            if (line->pb>0) OLEDDisplay_drawProgressBar(line->ekran,10,25,100,5,line->pb);
        } else OLEDDisplay_drawString(line->ekran,0, 22, line->satir[2]);
      }
    } else {
      if (line->kapali==true)
      {
          line->screen_minute = 0;
          OLEDDisplay_displayOn(line->ekran);
          line->off = false;
          line->kapali = false;
      }
      
      OLEDDisplay_setColor(line->ekran,WHITE);
      OLEDDisplay_drawString(line->ekran,0, 0, line->satir[0]);
      OLEDDisplay_drawString(line->ekran,0, 11, line->satir[1]);
      OLEDDisplay_drawString(line->ekran,0, 22, line->satir[3]);
    }

    OLEDDisplay_display(line->ekran);
}

void start_sen(void *arg)
{
    uint8_t *aa = (uint8_t *)arg;
    ESP_LOGW(TAG,"TIMER TRIG START %d",*aa);
    if (*aa!=99)
    {
        senaryo_t *tmp = get_senaryo_handle();
        while(tmp)
        {       
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "T_SEN");
            cJSON_AddStringToObject(root, "uname", tmp->name);
            cJSON_AddStringToObject(root, "ack", tmp->act);
            char *dat = cJSON_PrintUnformatted(root);
            while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
            uart.Sender(dat,RS485_MASTER);
            vTaskDelay(50/portTICK_PERIOD_MS); 
            cJSON_free(dat);
            cJSON_Delete(root);   
            vTaskDelay(500/portTICK_PERIOD_MS);        
            tmp = (senaryo_t *)tmp->next;
        }
    } else {
        //reset senaryosunu uygula
        //Öncelikle CPU2 ye bir reset bilgisi gönder
        ESP_LOGW(TAG,"RESET Senaryosu uygulanıyor..");
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "T_SEN");
            cJSON_AddStringToObject(root, "uname", "RESET");
            cJSON_AddStringToObject(root, "ack", "RESET");
            char *dat = cJSON_PrintUnformatted(root);
            while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
            uart.Sender(dat,RS485_MASTER);
            vTaskDelay(50/portTICK_PERIOD_MS); 
            cJSON_free(dat);
            cJSON_Delete(root); 
        vTaskDelay(10000/portTICK_PERIOD_MS);     
        //önce cpu 2 yi resetle ardından kendini resetle     
    } 
}
void stop_sen(void *arg)
{
    uint8_t *aa = (uint8_t *)arg;
    ESP_LOGW(TAG,"TIMER TRIG STOP %d",*aa);
    senaryo_t *tmp = get_senaryo_handle();
    while(tmp)
     {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "T_SEN");
        cJSON_AddStringToObject(root, "uname", tmp->name);
        cJSON_AddStringToObject(root, "ack", tmp->pas);
        char *dat = cJSON_PrintUnformatted(root);
        uart.Sender(dat,RS485_MASTER);
        cJSON_free(dat);
        cJSON_Delete(root);    
        vTaskDelay(500/portTICK_PERIOD_MS);  
        tmp = (senaryo_t *)tmp->next;
     }
}

static void senaryo_tim_callback(void* arg)
{
    trig.clear_callbacks();
    //printf("Before Callback Count %d\n",trig.count_callbacks());
    senaryo_read(GlobalConfig.aktif_senaryo,&trig,&start_sen,&stop_sen,disk);
    ESP_LOGI(TAG,"Timer Callback Count %d",trig.count_callbacks());
}

#include "lib/event.cpp"
#include "lib/tcpcallback.cpp"
#include "lib/http.c"
#include "lib/uartcallback.cpp"


/*
       PRE DEFINE
*/
void network_default_config(void);
void global_default_config(void);


void day_change(struct tm *tminfo)
{
   ESP_LOGW(TAG,"GUN DEGISTI"); 
}

void min_change(struct tm *tminfo)
{
   if (line->update)
   {
      char buf[64];
      strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", tminfo);
      if (!init_status) {
        strcpy(line->satir[1],buf);
        sceen_push();
      }
   }
   line->screen_minute++;
   
   //printf("%d %d\n",line->screen_minute, line->kapali);

   if (line->screen_minute>SCREEN_OFF_TIME && line->kapali==false) {
    OLEDDisplay_displayOff(line->ekran);
    line->screen_minute = 99;
    line->kapali=true;
   }
}


void CPU2_Reset(void)
{
   
   //if (++cpu2_reset_counter<3)
   if (++cpu2_reset_counter<3)
   {
    gpio_set_level((gpio_num_t)CPU2_RESET, 1);
    vTaskDelay(500/portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)CPU2_RESET, 0);
    vTaskDelay(5000/portTICK_PERIOD_MS);
   } else {
    bool ff=true;
    line->update=false;
    strcpy(line->satir[0],"CIHAZ DURDURULDU");
    strcpy(line->satir[1],"ERROR : CPU2 DEAD");
    strcpy(line->satir[2],"");
    sceen_push();
    while(true)
    {
        gpio_set_level((gpio_num_t)LLED0, ff);
        gpio_set_level((gpio_num_t)LLED1, ff);
        gpio_set_level((gpio_num_t)LLED2, ff);
        vTaskDelay(200/portTICK_PERIOD_MS);
        ff=!ff;
    }
   }
}

void CPU2_Ping_Reset(uint8_t counter)
{
  if (GlobalConfig.reset_servisi==1)
  {  
    ESP_LOGW(TAG,"CPU2 Cevap vermiyor %d",counter);
    if (counter>9) ESP_LOGE(TAG,"CPU2 RESETLENECEK");
    if (counter>10 && GlobalConfig.reset_servisi==1) {
        ESP_LOGE(TAG,"CPU2 RESETLIYORUM");
        CPU2_Reset();
    }
  }
}

void defreset(void)
{
    network_default_config();
    global_default_config();
}

void webwrite(void)
{
    disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

static void self_reset(void *arg)
{
    if (GlobalConfig.start_value!=1 && GlobalConfig.reset_servisi==1)
    {
        GlobalConfig.start_value = 1; 
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
        printf("Kendini resetlettir\n");
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "reset");
        char *dat = cJSON_PrintUnformatted(root); 
            while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
            uart.Sender(dat,RS485_MASTER);
            vTaskDelay(50/portTICK_PERIOD_MS); 
        
        cJSON_free(dat);
        cJSON_Delete(root);  
    }
}

bool time_read(struct tm *aa)
{
  esp_log_level_set("i2cdev", ESP_LOG_NONE);
  bool valid=false;
  uint8_t x = 0;
    for (x=0;x<5;x++)
    {
       if (pcf8563_get_time(&i2cbus, aa, &valid)==ESP_OK) x=88;
       vTaskDelay(100/portTICK_PERIOD_MS);
    }   
    esp_log_level_set("i2cdev", ESP_LOG_VERBOSE);
    return valid;
}
bool time_write(struct tm *aa)
{
  esp_log_level_set("i2cdev", ESP_LOG_NONE);  
  bool valid=false;
  uint8_t x = 0;
    for (x=0;x<5;x++)
    {
       if (pcf8563_set_time(&i2cbus,aa)==ESP_OK) x=88;
       vTaskDelay(100/portTICK_PERIOD_MS);
    }   
    if (x==88) valid=true;
    esp_log_level_set("i2cdev", ESP_LOG_VERBOSE);
    return valid;
}
void time_sync(struct timeval *tv)
{
    time_t now;
	  struct tm tt1;
    struct tm tt0;
    bool valid=false;
	time(&now);
	localtime_r(&now, &tt1);
	setenv("TZ", "UTC-03:00", 1);
	tzset();
	localtime_r(&now, &tt1);
    
    valid = time_read(&tt0);
    if (!valid)
      {
        tt0.tm_sec = tt1.tm_sec;
        tt0.tm_min = tt1.tm_min;
        tt0.tm_hour = tt1.tm_hour;
        tt0.tm_mday = tt1.tm_mday;
        tt0.tm_mon = tt1.tm_mon;
        tt0.tm_year = tt1.tm_year;
        time_write(&tt0);
      }
    valid = false;  
    if (tt0.tm_sec != tt1.tm_sec) {tt0.tm_sec = tt1.tm_sec;valid=true;}
    if (tt0.tm_min != tt1.tm_min) {tt0.tm_min = tt1.tm_min;valid=true;}
    if (tt0.tm_hour != tt1.tm_hour) {tt0.tm_hour = tt1.tm_hour;valid=true;}
    if (tt0.tm_mday != tt1.tm_mday) {tt0.tm_mday = tt1.tm_mday;valid=true;}
    if (tt0.tm_mon != tt1.tm_mon) {tt0.tm_mon = tt1.tm_mon;valid=true;}
    if (tt0.tm_year != tt1.tm_year) {tt0.tm_year = tt1.tm_year;valid=true;}
    if (valid) time_write(&tt0);
    valid=time_read(&tt0);   
}

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
void ota_task(void *param)
{
    home_network_config_t *cfg = (home_network_config_t *) param; 
    ESP_LOGI(TAG, "Starting Firmware UPGRADE TASK");
       
    char *srv = (char*)malloc(100);
    sprintf(srv,"http://%s:8043/iCe_CPU1.bin",cfg->update_server);
    ESP_LOGI(TAG,"Update server URL : %s",srv);

    esp_http_client_config_t config = {
        .url = srv,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    esp_err_t ret = esp_https_ota(&ota_config);
    cfg->upgrade = 0;
    disk.write_file(NETWORK_FILE,cfg,sizeof(NetworkConfig),0);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(TAG, "FIRMWARE UPGRADE FAILED..");
    }
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

#include "lib/mqttcallback.cpp"
#include "lib/config.cpp"

void startping(void *arg)
{
  ESP_LOGE(TAG, "PING SYSTEM START");
  ping_system((char *)NetworkConfig.console_ip, GlobalConfig.ping_time);
}

#include "lwip/dns.h"
//#include "lwip/ip4_addr.h

void query_dns(const char *name)
{
    struct addrinfo *address_info;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(name, NULL, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(TAG, "couldn't get hostname for :%s: "
                      "getaddrinfo() returns %d, addrinfo=%p", name, res, address_info);
    } else {
        if (address_info->ai_family == AF_INET) {
            struct sockaddr_in *p = (struct sockaddr_in *)address_info->ai_addr;
            ESP_LOGI(TAG, "Resolved %s IPv4 address: %s", name,ipaddr_ntoa((const ip_addr_t*)&p->sin_addr.s_addr));
        }
    }    
}

int log_file(const char *fmt, va_list args) {
   // printf("log \n");
   //Loglar buraya yönlendirildi. 
    return vprintf(fmt, args);
}

extern "C" void app_main()
{
    esp_log_level_set("*", ESP_LOG_VERBOSE); 
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);   
    esp_log_level_set("esp-tls", ESP_LOG_NONE);  
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_NONE); 
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_NONE); 
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE); 
    esp_log_level_set("enc28j60", ESP_LOG_NONE); 
    esp_log_level_set("esp_eth.netif.netif_glue", ESP_LOG_NONE); 

    // ESP_LOGI(TAG, "***Redirecting log output to SPIFFS log file (also keep sending logs to UART0)");
    //esp_log_set_vprintf(&log_file);
  

    config();

    senaryo_tim_callback(NULL);
    uint8_t cur_sen = GlobalConfig.aktif_senaryo;
    uint8_t scr_stat = 0;


    if (GlobalConfig.ping_time>0) {

      esp_timer_handle_t ztimer = NULL;
      esp_timer_create_args_t arg1 = {};
      arg1.callback = &startping;
      arg1.name = "tpoll";
      ESP_ERROR_CHECK(esp_timer_create(&arg1, &ztimer));
      ESP_ERROR_CHECK(esp_timer_start_once(ztimer, 60000000));      
    }

    query_dns("icemqtt.com.tr");
      
    

    while(true) 
    {
        if(GlobalConfig.aktif_senaryo!=cur_sen) {cur_sen = GlobalConfig.aktif_senaryo;senaryo_tim_callback(NULL);} 

       if (gpio_get_level((gpio_num_t)DEF_RESET_BUTTON)==0)
         {
            uint64_t startMicros = esp_timer_get_time();
            uint64_t currentMicros = startMicros;
            //Eth.reset();
            ESP_LOGI(TAG,"TUS1 Basıldı");

            bool def_res = false;
            //if (line->off) {
              line->screen_minute = 0;  
              OLEDDisplay_displayOn(ekran);              
              line->off=false;
            //}
            
            line->screen_minute = 0;
            while (gpio_get_level((gpio_num_t)DEF_RESET_BUTTON)==0)
              {
                  currentMicros = esp_timer_get_time()-startMicros;
                  if ((currentMicros)>15000000 && !def_res) {
                          //Default Reset Yapılacak 
                          line->update=false;
                          strcpy(line->satir[0],"    TUSU BIRAKINIZ !....");
                          strcpy(line->satir[1],"Cihaz varsayilan degerlerle");
                          strcpy(line->satir[2],"     RESETLENECEK.");
                          sceen_push();
                          def_res = true;
                                                }   
                  vTaskDelay(10/portTICK_PERIOD_MS);              
              }

            if (currentMicros>500000 && currentMicros<9000000) 
              {
                 scr_stat++;
                 if (scr_stat>2) scr_stat=0;
                 if (scr_stat==0) sprintf(line->satir[2],"MAC : %02X:%02X:%02X:%02X:%02X:%02X",
                                                NetworkConfig.mac[0],
                                                NetworkConfig.mac[1],
                                                NetworkConfig.mac[2],
                                                NetworkConfig.mac[3],
                                                NetworkConfig.mac[4],
                                                NetworkConfig.mac[5]
                                            ); 
                if (scr_stat==1) sprintf(line->satir[2],"LIC:%s",GlobalConfig.license);
                if (scr_stat==2) sprintf(line->satir[2],"MQTT : %s",GlobalConfig.mqtt_server);
                sceen_push();                            
              }  
            if (def_res) {
                defreset();
                vTaskDelay(500/portTICK_PERIOD_MS);  
                esp_restart();
            }
            
         }
      vTaskDelay(10/portTICK_PERIOD_MS);
    }

}

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s %s", label, hash_print);
}

void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}

void network_default_config(void)
{
     NetworkConfig.home_default = 1; 
     NetworkConfig.wifi_type = HOME_ETHERNET;         
     NetworkConfig.ipstat = DYNAMIC_IP;
     strcpy((char*)NetworkConfig.wifi_ssid, "ice");
     strcpy((char*)NetworkConfig.wifi_pass, "iceice");
     strcpy((char*)NetworkConfig.ip,"192.168.10.1");
     strcpy((char*)NetworkConfig.netmask,"255.255.0.0");
     strcpy((char*)NetworkConfig.gateway,"192.168.10.1");

     strcpy((char*)NetworkConfig.dns,"195.175.39.39");
     strcpy((char*)NetworkConfig.backup_dns,"8.8.8.8");
     strcpy((char*)NetworkConfig.console_ip,"192.168.250.15");
     strcpy((char*)NetworkConfig.update_server,"icemqtt.com.tr");

     NetworkConfig.mac[0]=0x0A;
     NetworkConfig.mac[1]=0x00;
     NetworkConfig.mac[2]=0x00;
     NetworkConfig.mac[3]=0x12;
     NetworkConfig.mac[4]=0x02;
     NetworkConfig.mac[5]=0x0B;
     NetworkConfig.upgrade = 0;

/*
    NetworkConfig.wifi_type = HOME_WIFI_STA;
    strcpy((char *)NetworkConfig.wifi_ssid,(char *)"iCe_Teknik");
    strcpy((char *)NetworkConfig.wifi_pass,(char *)"cedetas2015");

    NetworkConfig.wifi_type = HOME_WIFI_STA;
    strcpy((char *)NetworkConfig.wifi_ssid,(char *)"VODAFONE_DED7");
    strcpy((char *)NetworkConfig.wifi_pass,(char *)"5066a12b");

    NetworkConfig.wifi_type = HOME_WIFI_STA;
    strcpy((char *)NetworkConfig.wifi_ssid,(char *)"IMS_YAZILIM");
    strcpy((char *)NetworkConfig.wifi_pass,(char *)"mer6514a4c");
*/

     disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
}

void global_default_config(void)
{
    //85.159.66.93
     GlobalConfig.home_default = 1; 
     strcpy((char*)GlobalConfig.device_name, "ICE_Main_Box");
     
     strcpy((char*)GlobalConfig.mqtt_server,"icemqtt.com.tr");
     strcpy((char*)GlobalConfig.license, "02.02.6579C727.F91D");
     GlobalConfig.device_id = 1;
     GlobalConfig.mqtt_keepalive = 60; 
     GlobalConfig.reset_servisi = 0; 
     GlobalConfig.start_value = 0; 
     GlobalConfig.blok = 97;
     GlobalConfig.daire = 1;
     GlobalConfig.http_start = 1;
     GlobalConfig.tcp_start = 1;
     GlobalConfig.location = 0;
     GlobalConfig.project_id = 0;
     GlobalConfig.random_mac = 1;
     GlobalConfig.rawmac[0] = 0x0A;
     GlobalConfig.rawmac[1] = 0x0A;
     GlobalConfig.rawmac[2] = 0x0A;
     GlobalConfig.rawmac[3] = 0x0A;
     GlobalConfig.ping_time = 0;
     GlobalConfig.reset_counter = 0;
     GlobalConfig.set_sntp = 0;
     
     disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}


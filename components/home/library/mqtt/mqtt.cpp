
#include "mqtt.h"
#include <string.h>
#include "cJSON.h"
#include "core.h"

static const char *TAG = "MQTT";

//ESP_EVENT_DEFINE_BASE(LED_EVENTS);

bool Mqtt::is_avaible(void)
{
    EventBits_t bits;
    if (Event!=NULL)
    {
        bits = xEventGroupGetBits(Event);
        if ((bits & MQTT_FAIL_BIT)) 
           return false;
        else return true;   
    }
    return false; 
}

bool Mqtt::publish(char* data, int len)
{
     if (is_avaible())
       {
        esp_mqtt_client_publish(client, sendtopic, data, len, 1, 0);
        ESP_LOGI(TAG, "MQTT Server Sended..");
        return true;
       }
    return false;   
} 


bool Mqtt::init(
            home_global_config_t cnf, 
            mqtt_callback_t messagecb,
            change_callback_t selfrescb
              )
{
    mConfig = cnf;
   // mqttcfg.host = (const char *)mConfig.mqtt_server;
    mqttcfg.broker.address.port = 1883;
    char *aa = (char*)malloc(60);
    strcpy(aa,"mqtt://");
    strcat(aa,(char*)mConfig.mqtt_server);
    mqttcfg.broker.address.uri = (const char *)aa;
    mqttcfg.session.keepalive = mConfig.mqtt_keepalive;
    mqttcfg.session.protocol_ver=  MQTT_PROTOCOL_V_3_1_1;
    message_callback = messagecb;
    selfreset_callback = selfrescb;
    Event = xEventGroupCreate();
    xEventGroupClearBits(Event, MQTT_CONNECTED_BIT | MQTT_FAIL_BIT | MQTT_SEND_BIT);
    
    set_license((char*)mConfig.license);
    return true;
}

/*
void Mqtt::set_messagebit(void)
{
   xEventGroupSetBits(Event, SEND_MESSAGE_BIT); 
}

void Mqtt::reset_messagebit(void)
{
  xEventGroupClearBits(Event, SEND_MESSAGE_BIT); 
}

bool Mqtt::is_messagebit(void)
{
   EventBits_t bits = xEventGroupGetBits( Event );
   if (bits & SEND_MESSAGE_BIT) 
   {
    return true;
   } else {
    return false;
   }
}
*/
void Mqtt::send_ack(int id)
{
   if (is_avaible())
    {
        cJSON *rt1;
        cJSON *fmt1;
        rt1 = cJSON_CreateObject();
        cJSON_AddStringToObject(rt1, "com", "ack");
        cJSON_AddNumberToObject(rt1, "id", id);
            
        cJSON_AddItemToObject(rt1, "durum", fmt1=cJSON_CreateObject());
        cJSON_AddTrueToObject(fmt1, "stat");
        char *dat = cJSON_PrintUnformatted(rt1); 
        publish(dat,strlen(dat));
        cJSON_free(dat);
        cJSON_Delete(rt1);
    }
}

void Mqtt::set_license(char *lic)
{
   clientname = (char*) calloc(1,64);
   strcpy((char *)clientname,(char*)lic);
   strcat((char *)clientname,"-DEV");
   mqttcfg.credentials.client_id = clientname;
   
   willtopic = (char*) calloc(1,64);
   listentopic = (char*) calloc(1,64);
   sendtopic = (char*) calloc(1,64);
   sharetopic = (char*) calloc(1,64);

   strcpy((char*)willtopic,(char*)"/");
   strcat((char*)willtopic,(char*)lic);
   strcat((char*)willtopic,"/devWill");
   mqttcfg.session.last_will.topic = willtopic;
   mqttcfg.session.last_will.qos=1;
   mqttcfg.session.last_will.retain = 1;
   mqttcfg.session.keepalive=mConfig.mqtt_keepalive;

	Willjson = cJSON_CreateObject();
    cJSON_AddStringToObject(Willjson, "com", "will");
    cJSON_AddStringToObject(Willjson, "status", "off");
    cJSON_AddNumberToObject(Willjson, "location", mConfig.location);

            time_t now;
            struct tm tt0;
            time(&now);
	        localtime_r(&now, &tt0);
            
    char *rr = (char *)malloc(50);
    memset((char*)rr,0,48);
    strftime(rr, 50, "%d.%m.%Y %H:%M", &tt0);
    cJSON_AddStringToObject(Willjson, "active", rr);

    mqttcfg.session.last_will.msg = cJSON_PrintUnformatted(Willjson);
    mqttcfg.session.last_will.msg_len = strlen(mqttcfg.session.last_will.msg);
    free(rr);

   strcpy((char*)listentopic,(char*)"/");
   strcat((char*)listentopic,(char*)lic);
   strcat((char*)listentopic,"/devListener");

   strcpy((char*)sendtopic,(char*)"/");
   strcat((char*)sendtopic,(char*)lic);
   strcat((char*)sendtopic,"/devSender");

   strcpy((char*)sharetopic,(char*)"/");
   strcat((char*)sharetopic,(char*)"$share/");
   char *tmp = (char*) calloc(1,5);
   memcpy(tmp,lic,2);
   strcat((char*)sharetopic,(char*)tmp);
   strcat((char*)sharetopic,"/ice");
   free(tmp); 

    ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG, "  Mqtt Client   : %s", mqttcfg.credentials.client_id); 
    ESP_LOGI(TAG, "  Server        : %s", mqttcfg.broker.address.uri); 
    ESP_LOGI(TAG, "  Will Topic    : %s", mqttcfg.session.last_will.topic); 
    ESP_LOGI(TAG, "  Send Topic    : %s", sendtopic); 
    ESP_LOGI(TAG, "  Listen Topic  : %s", listentopic); 
    ESP_LOGI(TAG, "  share Topic   : %s", sharetopic); 
    ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
   // ESP_LOGE(TAG, "MQTT event loop base=%s, event_id=%d", base, event_id);
    led_events_data_t ld={};
    Mqtt *self = (Mqtt *) (handler_args);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    if ((esp_mqtt_event_id_t)event_id==MQTT_EVENT_CONNECTED)
    {
        //will topigine cihazın açıldığı bildiriliyor
        cJSON *rt1;
	      rt1 = cJSON_CreateObject();
          cJSON_AddStringToObject(rt1, "com", "will");
          cJSON_AddStringToObject(rt1, "status", "on");
          cJSON_AddNumberToObject(rt1, "location", self->mConfig.location);
          cJSON_AddNumberToObject(rt1, "blok", self->mConfig.blok);
          cJSON_AddNumberToObject(rt1, "daire", self->mConfig.daire);
          cJSON_AddNumberToObject(rt1, "proje", self->mConfig.project_id);

            time_t now;
            struct tm tt0;
            time(&now);
	        localtime_r(&now, &tt0);
            char *rr = (char *)malloc(50);
            memset((char*)rr,0,48);
            strftime(rr, 50, "%d.%m.%Y %H:%M", &tt0);
            cJSON_AddStringToObject(rt1, "active", rr);

          char *dat = cJSON_PrintUnformatted(rt1);
          esp_mqtt_client_publish(client, self->willtopic, dat, strlen(dat), 1, 1);
          cJSON_free(dat);
          cJSON_Delete(rt1);
        //listen topige baglanıyor
        esp_mqtt_client_subscribe(client, self->listentopic, 1);
        //share topige baglanıyor
        //ESP_LOGI(TAG, "  share Topic   : %s", self->sharetopic);  
        esp_mqtt_client_subscribe(client, self->sharetopic, 0);

        xEventGroupSetBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupClearBits(self->Event, MQTT_FAIL_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED and SUBSRIBED");
        ld.state = 0;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
    } else 
    switch ((esp_mqtt_event_id_t)event_id) {
        /*
    case MQTT_EVENT_CONNECTED:        
        //will topigine cihazın açıldığı bildiriliyor
        char *mm = (char *)malloc(50);
        strcpy(mm,"{'com':'will', 'status':'on'}");
        //cJSON *rt1;
	    //  rt1 = cJSON_CreateObject();
        //  cJSON_AddStringToObject(rt1, "com", "will");
        //  cJSON_AddStringToObject(rt1, "status", "on");
        //  char *dat = cJSON_PrintUnformatted(rt1);
          esp_mqtt_client_publish(client, self->willtopic, mm, strlen(mm), 1, 1);
          free(mm);
          //cJSON_free(dat);
          //cJSON_Delete(rt1);
        //listen topige baglanıyor
        esp_mqtt_client_subscribe(client, self->listentopic, 1);
        //share topige baglanıyor
        //ESP_LOGI(TAG, "  share Topic   : %s", self->sharetopic);  
        esp_mqtt_client_subscribe(client, self->sharetopic, 0);

        xEventGroupSetBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupClearBits(self->Event, MQTT_FAIL_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED and SUBSRIBED");
        ld.state = 0;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
        break;
        */
    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupSetBits(self->Event, MQTT_FAIL_BIT);
        ld.state = 1;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
        ESP_LOGE(TAG, "MQTT DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        xEventGroupClearBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupSetBits(self->Event, MQTT_FAIL_BIT);
        ld.state = 1;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
        ESP_LOGE(TAG, "MQTT UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        self->message_callback(event->topic, event->data, event->topic_len, event->data_len);       
        break;
    case MQTT_EVENT_ERROR:
         
         ESP_LOGE(TAG, "MQTT_EVENT_ERROR %d %d", event->error_handle->error_type, ++self->error_counter);
         if (self->error_counter>10) if (self->selfreset_callback!=NULL) self->selfreset_callback(self);
         /*
         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            //ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            
        } 
        */
        xEventGroupClearBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupSetBits(self->Event, MQTT_FAIL_BIT);
        ld.state = 1;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
        break;
    default:
      if (event->event_id!=7)
      {
        ESP_LOGE(TAG, "MQTT Other event id:%d", event->event_id);
        xEventGroupClearBits(self->Event, MQTT_CONNECTED_BIT);
        xEventGroupSetBits(self->Event, MQTT_FAIL_BIT);
        ld.state = 1;
        ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));
      }
        break;
    }
}


bool Mqtt::start(void)
{

        //Herhangi bir transmisyon çalışır durumda ise
        //led_events_data_t ld={};
        //ld.state = 1;
        //ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_MQTT, &ld, sizeof(led_events_data_t), portMAX_DELAY));

        client = esp_mqtt_client_init(&mqttcfg);
        if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, this)!=ESP_OK) return false;
        if (esp_mqtt_client_start(client)!=ESP_OK) return false;
        //connect veya fail olana kadar bekle 
        xEventGroupWaitBits(Event,
                MQTT_CONNECTED_BIT | MQTT_FAIL_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY);
        //client oluşturuldu. Fail olsa bile tekrar denemeye devam edecek. 
        error_counter = 0;       
        return true; 
}
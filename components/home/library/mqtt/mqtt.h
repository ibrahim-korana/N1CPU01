#ifndef _MQTT_H
#define _MQTT_H

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "core.h"

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT      BIT1
#define MQTT_SEND_BIT   BIT2

class Mqtt {
    public:
      Mqtt() {};
      ~Mqtt() {};

      bool init(
            home_global_config_t cnf,
            mqtt_callback_t messagecb,
            change_callback_t selfrescb
           );

      bool start(void);
      bool publish(char *data, int len);
      void set_messagebit(void);
      void reset_messagebit(void);
      bool is_messagebit(void);
      void send_ack(int id);

     
      const char* sendtopic; 
      const char* willtopic;
      const char* listentopic;
      const char* sharetopic;
      const char* clientname;
      cJSON *Willjson;
      
      mqtt_callback_t message_callback; 
      change_callback_t selfreset_callback;

      EventGroupHandle_t Event;
      uint8_t error_counter = 0;
      home_global_config_t mConfig;

    private:
       
       
       esp_mqtt_client_config_t mqttcfg = {};
       esp_mqtt_client_handle_t client;   
       void set_license(char *lic);
       bool is_avaible(void);

       uint8_t error;
       uint8_t status;
           
    protected:  
        

};



#endif
#ifndef _LAMP_H
#define _LAMP_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

//Lamba Version 1.1

class Lamp : public Base_Function {
    public:
      Lamp(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"lamp");
        function_callback = cb;
        statustype = 1;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              write_status();
        }      
      };
      ~Lamp() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);  
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;

      void fire(bool stat);
      void ConvertStatus(home_status_t stt, cJSON* obj);

      uint8_t sure = 0;
      void xtim_stop(void);
      void xtim_start(void);  
      void senaryo(char *par);
        
    private:
      esp_timer_handle_t qtimer = NULL;
      esp_timer_handle_t xtimer = NULL;
      
    protected:  
      static void func_callback(void *arg, port_action_t action);     
      static void lamp_tim_callback(void* arg);    
      static void xtim_callback(void* arg);      
      void tim_stop(void);
      void tim_start(void);    
      
};

#endif
#ifndef _PRIZ_H
#define _PRIZ_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

class Priz : public Base_Function {
    public:
      Priz(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"priz");
        function_callback = cb;
        statustype = 1;
        disk = dsk;
        timer = 0;
        disk.read_status(&status,genel.device_id);
        status.counter = 0;
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
      ~Priz() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;

      void fire(bool stat);
      void senaryo(char *par);
      void set_start(bool drm); 
      void ConvertStatus(home_status_t stt, cJSON* obj);
      void tim_stop(void);
        
    private:
      esp_timer_handle_t qtimer = NULL; 
    protected:  
      static void func_callback(void *arg, port_action_t action); 
      static void priz_tim_callback(void* arg);     
      
      void tim_start(void);     
};

#endif

/*
       prizi açan status tanımı içinde stat=true ile birlikte
       counter özelliği ile dakika cinsinden bir değer gönderirseniz
       priz belirttiğiniz süre kadar açık kalır. Counter her bir 
       dakikada azaltılarak action oluşturulur. Süre sona erdiğinde 
       priz kapatılır. 
*/
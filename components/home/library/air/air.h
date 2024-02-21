#ifndef _AIR_H
#define _AIR_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "jsontool.h"


class Air : public Base_Function {
    public:
      Air(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"air");
        function_callback = cb;
        statustype = 1;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;        
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = true;
              status.set_temp = 25;
              status.active   = true;
              status.first    = true;
              write_status();
        }        
      };
      ~Air() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void remote_set_status(home_status_t stat, bool callback_call=true);
      void set_sensor(char *name, home_status_t stat);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;

      void fire(bool stat);
      void senaryo(char *par);
      //void set_start(bool drm); 
      void ConvertStatus(home_status_t stt, cJSON* obj);
      bool send_gateway=false;
        
    private:
    protected:  
      static void func_callback(void *arg, port_action_t action);  
      void temp_action(bool send=true);    
};

#endif
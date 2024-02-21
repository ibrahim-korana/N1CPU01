#ifndef _GAS_H
#define _GAS_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"


class Gas : public Base_Function {
    public:
      Gas(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"gas");
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
      ~Gas() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;

      void fire(bool stat);
      void senaryo(char *par);
      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
           
    protected:  
      static void func_callback(void *arg, port_action_t action);  
};

#endif
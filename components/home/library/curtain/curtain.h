#ifndef _CURTAIN_H
#define _CURTAIN_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

typedef enum {
    CUR_CLOSED = 0,
    CUR_HALF_CLOSED, 
    CUR_OPENED,
    CUR_PROCESS,
    CUR_UNKNOWN, 
} cur_status_t;

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_DOWN, 
    MOTOR_UP,
} motor_status_t;

typedef enum {
    CURBTN_UNKNOWN = 0,
    CURBTN_SHORT, 
    CURBTN_LONG,
} btn_stat_t;

class Curtain : public Base_Function {
    public:
      Curtain(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"cur");
        function_callback = cb;
        statustype = 4;
        disk = dsk;
        local_port_callback = &func_callback;
        disk.read_status(&status,genel.device_id);
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              write_status();
        }      
      };
      ~Curtain() {};

      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
      
      //Statusu json olarak dondurur
      void get_status_json(cJSON* obj) override;
      //değişebilir portları dondurur
      bool get_port_json(cJSON* obj) override;
      void init(void);

      void ConvertStatus(home_status_t stt, cJSON *obj);
      void fire(bool stat); 
      void senaryo(char *par);         

      uint8_t get_sure(void) {return sure;}
      void set_sure(uint8_t sr) {sure=sr;}   
      void motor_action(bool callback=true);


      cur_status_t temp_status = CUR_UNKNOWN;
      cur_status_t current_status = CUR_OPENED;
      motor_status_t Motor = MOTOR_STOP; 
      btn_stat_t buton_durumu = CURBTN_UNKNOWN;
      const char* convert_motor_status(uint8_t mot);

      bool eskitip=false;

      void tim_stop();
      void tim_start();

      void role_change(void);
      void anahtar_change(void);

    private:
      esp_timer_handle_t qtimer;
      uint8_t sure = 60;
    protected:  
       static void func_callback(void *arg, port_action_t action);  
       static void cur_tim_callback(void* arg);            
};

#endif
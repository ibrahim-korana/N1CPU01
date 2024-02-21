#include "gas.h"
#include "esp_log.h"

static const char *GAS_TAG = "GAS";

//Bu fonksiyon inporttan tetiklenir
void Gas::func_callback(void *arg, port_action_t action)
{   
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Gas::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        Base_Port *target = port_head_handle;
        //bool change=false;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.stat = target->set_status(status.stat);
                    //change=true;
                }
            target = target->next;
        }
        write_status();
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Gas::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=genel.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(GAS_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void Gas::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Gas::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Gas::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Gas::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //Gaz yangın ihbarında kapatılır 
        home_status_t ss = get_status();
        ss.stat = false;
        set_status(ss);
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void Gas::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.stat = true; 
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.stat = false; 
        set_status(status);
      }  
}

void Gas::init(void)
{
    if (!genel.virtual_device)
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type==PORT_OUTPORT)
                {
                status.stat = target->get_hardware_status();
                break;
                }
            target=target->next;
        }
    }
}

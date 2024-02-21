#include "water.h"
#include "esp_log.h"

static const char *WATER_TAG = "WATER";

//Bu fonksiyon inporttan tetiklenir
void Water::func_callback(void *arg, port_action_t action)
{   
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Water::set_status(home_status_t stat)
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
void Water::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=genel.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(WATER_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void Water::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Water::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Water::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Water::fire(bool stat)
{
    //Su yangında birşey yapmaz
}

void Water::senaryo(char *par)
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

void Water::init(void)
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

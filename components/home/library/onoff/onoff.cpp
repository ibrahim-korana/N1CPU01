
#include "onoff.h"
#include "esp_log.h"

static const char *ONOFF_TAG = "ONOFF";

//Bu fonksiyon inporttan tetiklenir
void Onoff::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Base_Function *lamba = (Base_Function *) arg;
        home_status_t stat = lamba->get_status();
        bool change = false;
        if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
           {
              //portları tarayıp çıkış portlarını bul
              Base_Port *target = lamba->port_head_handle;
              while (target) {
                if (target->type == PORT_OUTPORT) 
                  {
                     if (prt->type==PORT_INPORT) {
                        if (evt==BUTTON_PRESS_DOWN) {stat.stat = target->set_status(true);change = true;}
                        if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_status(false);change = true;}
                     }
                     if (prt->type==PORT_INPULS) {
                        if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_toggle();change = true;}
                     }
                  }       
                target = target->next;
                            }
           }
        if (change) {
            ((Onoff *)lamba)->local_set_status(stat,true);
            if (lamba->function_callback!=NULL) lamba->function_callback(lamba, lamba->get_status());
        } 
     }
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Onoff::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        Base_Port *target = port_head_handle;
       // bool change=false;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.stat = target->set_status(status.stat);
                   // change=true;
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
void Onoff::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=stat.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(ONOFF_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Onoff::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Onoff::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Onoff::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Onoff::fire(bool stat)
{
}

void Onoff::senaryo(char *par)
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
void Onoff::init(void)
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


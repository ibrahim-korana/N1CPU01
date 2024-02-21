#include "mwater.h"
#include "esp_log.h"

static const char *MWATER_TAG = "MWATER";

//Bu fonksiyon inporttan tetiklenir
void MWater::func_callback(void *arg, port_action_t action)
{   
}

void MWater::timer_callback(void* arg)
{
    MWater *mthis = (MWater *)arg;
    Base_Port *target = mthis->port_head_handle;
        //bool change=false;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(false);
                }
            target = target->next;
        }
    mthis->status.status = 0;   
    if (mthis->function_callback!=NULL) mthis->function_callback(arg, mthis->get_status()); 
}

void MWater::tim_start(void)
{
  ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
}

void MWater::status_on(void)
{
    if (!esp_timer_is_active(qtimer)) 
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT && strcmp(target->name,"ON")==0) 
                {
                    target->set_status(true);
                }
            target = target->next;
        } 
        tim_start();
        status.status = 1;
    }
}
void MWater::status_off(void)
{
  if (!esp_timer_is_active(qtimer)) 
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT && strcmp(target->name,"OFF")==0) 
                {
                    target->set_status(true);
                }
            target = target->next;
        } 
        tim_start();
        status.status = 1;
    }
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void MWater::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        if (status.stat) status_on();
        if (!status.stat) status_off();
        write_status();
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void MWater::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=stat.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(MWATER_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void MWater::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void MWater::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool MWater::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void MWater::fire(bool stat)
{
    //Su yangında birşey yapmaz
}

void MWater::senaryo(char *par)
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

void MWater::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &timer_callback;
        arg.name = "tim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (timer==0 || timer>60) timer=10;
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(false);
                }
            target = target->next;
        }
        status.status = 0;   
    }
}

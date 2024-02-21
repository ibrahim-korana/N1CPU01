#include "TmRole.h"
#include "esp_log.h"

#define KAPALI false
#define ACIK true

static const char *TMRELAYTAG_TAG = "TMRELAY";

//Bu fonksiyon inporttan tetiklenir
void TmRelay::func_callback(void *arg, port_action_t action)
{   
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void TmRelay::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        Base_Port *target = port_head_handle;
       //printf("TMROLE status %d\n",status.stat);
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.stat = target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        //if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        //if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void TmRelay::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    //if (status.active!=genel.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(TMRELAYTAG_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void TmRelay::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    //if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void TmRelay::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool TmRelay::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void TmRelay::fire(bool stat)
{
    
}

void TmRelay::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.stat = ACIK; 
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.stat = KAPALI; 
        set_status(status);
      }  
    if(strcmp(par,"RESET")==0)
      {
        status.stat = ACIK; 
        set_status(status);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        status.stat = KAPALI; 
        set_status(status);
      }    
}

void TmRelay::tim_callback(void* arg)
{   
    Base_Function *aa = (Base_Function *)arg;
    home_status_t ss = aa->get_status();
    ss.stat = ACIK;
    aa->set_status(ss);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ss.stat = KAPALI;
    aa->set_status(ss);

}

void TmRelay::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void TmRelay::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_periodic(qtimer, timer * 60000000));
}

void TmRelay::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (timer>0) tim_start();
        status.stat = KAPALI;
        set_status(status);
        //printf("INIT TMROLE status %d\n",status.stat);
    }
}

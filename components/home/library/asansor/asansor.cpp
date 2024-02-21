
#include "asansor.h"
#include "esp_log.h"

static const char *ASANSOR_TAG = "ASANSOR";

//Bu fonksiyon inporttan tetiklenir
void Asansor::func_callback(void *arg, port_action_t action)
{   
   
}

void Asansor::status_change(bool st)
{
  Base_Port *target = port_head_handle;
  while (target) {
      if (target->type == PORT_OUTPORT) 
          {
              target->set_status(st);
          }
      target = target->next;
  }
  if (st) status.status=1; else status.status=0;
  write_status();
  if (function_callback!=NULL) function_callback((void *)this, get_status());
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Asansor::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        stat.stat = true;
        local_set_status(stat);
        status_change(true);
        tim_start();
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Asansor::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=stat.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(ASANSOR_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Asansor::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void Asansor::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Asansor::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Asansor::fire(bool stat)
{
}

void Asansor::senaryo(char *par)
{
   
}

void Asansor::tim_callback(void* arg)
{   
    Asansor *aa = (Asansor *)arg;
    aa->status_change(false);
}

void Asansor::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Asansor::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
}
void Asansor::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (timer==0) timer=3;
    }
}


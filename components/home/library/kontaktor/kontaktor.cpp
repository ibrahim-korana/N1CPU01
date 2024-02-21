#include "kontaktor.h"
#include "esp_log.h"

static const char *KONT_TAG = "KONTAKTOR";

//Bu fonksiyon inporttan tetiklenir
void Contactor::func_callback(void *arg, port_action_t action)
{   
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Contactor::set_status(home_status_t stat)
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
void Contactor::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    //if (status.active!=genel.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(KONT_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void Contactor::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    //if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Contactor::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Contactor::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Contactor::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //kontaktor yangın ihbarında gecikmeli kapatılır 
        tim_start();
    } else {
        tim_stop();
       status = main_temp_status;
       set_status(status);       
    }
}

void Contactor::tim_callback(void* arg)
{   
    Base_Function *aa = (Base_Function *)arg;
    home_status_t ss = aa->get_status();
    ss.stat = false;
    aa->set_status(ss);
}

void Contactor::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Contactor::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
}

void Contactor::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (timer==0) timer=120;
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

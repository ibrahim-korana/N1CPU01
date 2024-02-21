#include "priz.h"
#include "esp_log.h"

static const char *PRIZ_TAG = "PRIZ";

//Bu fonksiyon inporttan tetiklenir
void Priz::func_callback(void *arg, port_action_t action)
{   
}

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Priz::set_status(home_status_t stat)
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
                    if (stat.counter>0) tim_start();
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
void Priz::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=genel.active) chg=true;
    if (status.counter!=stat.counter) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(PRIZ_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Priz::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter>0) cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Priz::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Priz::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Priz::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //prizler yangın ihbarında kapatılır
        tim_stop();
        status.stat = false;
        set_status(status);
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void Priz::senaryo(char *par)
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
void Priz::priz_tim_callback(void* arg)
{   
    //çıkışı kapat
    Base_Function *aa = (Base_Function *) arg;
    home_status_t stat = aa->get_status();
    if (stat.counter>0) stat.counter--;
    if (stat.counter==0)
    {     
        Base_Port *target = aa->port_head_handle;
        ((Priz*)aa)->tim_stop();
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    stat.stat = target->set_status(false);
                }
            target = target->next;
        }
    }
    aa->local_set_status(stat,true);
    if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
}

void Priz::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Priz::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_periodic(qtimer, 60000000));
}

void Priz::init(void)
{
    status.counter = 0;
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &priz_tim_callback;
        arg.name = "Ptim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer));
        //çıkış portu okunuyor
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

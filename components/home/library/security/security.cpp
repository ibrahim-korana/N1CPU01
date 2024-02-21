#include "security.h"
#include "esp_log.h"

static const char *SEC_TAG = "SECURITY";

//Bu fonksiyon inporttan tetiklenir
void Security::func_callback(void *arg, port_action_t action)
{ 
    if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Base_Function *fun = (Base_Function *) arg;
        home_status_t stat = fun->get_status();
        /*
            sensor mesajları alarm açıksa değerlendirilsin
        */
        bool acc = (stat.status==SEC_OPENED);
        //Acil portlarda alarm kapalı olsa da alarm ver
        if (prt->type==PORT_FIRE || prt->type==PORT_GAS || prt->type==PORT_EMERGENCY || prt->type==PORT_WATER) acc=true;
        //tek şart kullanıcının portu iptal etmemiş olması  
        bool strt = (acc) && (prt->get_user_active());
        printf("%s STRT DURUMU=%d %d&%d\n",prt->name,stat.status,(acc), prt->get_user_active());
       // 
        if (strt)
        {
            bool change = false;
            if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
            {
                prt->set_active_admin(false);
                prt->set_alarm(true);
                stat.status = SEC_ALARM;
                strcpy((char *)stat.ircom,prt->name);  //ircom üzerinden alarm port adı
                stat.color = prt->type; //color üzerinden portun type  
                ((Security *)fun)->cikis_port_active();
                change = true;
            }
            if (change) {
                ((Security *)fun)->local_set_status(stat,true);
                if (fun->function_callback!=NULL) fun->function_callback(fun, fun->get_status());
            } 
        }
     }
}

void Security::cikis_port_active(void)
{
   Base_Port *target = port_head_handle;
    while (target) 
    {
        if (target->type == PORT_OUTPORT) //port giriş portu ise
        { 
            if (target->get_user_active()) //kullanıcı iptal etmemişse
              target->set_status(true); //aktif yap
        }
        target = target->next;
    }
}

void Security::alarm_open(void)
{
   // status.stat = true;
    status.status = (int)SEC_OPENED;
    ESP_LOGW(SEC_TAG,"ALARM OPEN");
    //aktif portlar start yapılıyor 
    Base_Port *target = port_head_handle;
    while (target) 
    {
        target->set_alarm(false); //Tüm alarm işaretleri false oluyor 
        if (target->type != PORT_OUTPORT) //port giriş portu ise
        {
            if (target->get_user_active()) //kullanıcı iptal etmemişse
              {
                target->set_active(true); //aktif yap
              }
        }
        target = target->next;
    }  
    write_status();
    if (function_callback!=NULL) function_callback((void *)this, get_status());
}

/*
   ALARM Çıkış portunu kapatır.
*/
void Security::alarm_stop(void)
{
    status.status = (int)SEC_STOP;
    ESP_LOGW(SEC_TAG,"ALARM STOP");
    //aktif portlar stop yapılıyor 
    Base_Port *target = port_head_handle;
    while (target) 
    {
         if (target->type == PORT_OUTPORT)
         {
            //alarm portunu durdur
            target->set_status(false);
         }
        target = target->next;
    }  
    write_status();
}

/*
    Alarm çıkış portunu ve Acil portlar hariç 
    diğer portları durdurur
*/
void Security::alarm_close(void)
{
  //  status.stat = false;
    status.status = (int)SEC_CLOSED;
    ESP_LOGW(SEC_TAG,"ALARM CLOSE");
    //aktif portlar stop yapılıyor 
    Base_Port *target = port_head_handle;
    while (target) 
    {
        if (target->type==PORT_OUTPORT) 
        {
            target->set_status(false); //portu kapat 
        }
   
        if (target->get_user_active()) //kullanıcı iptal etmemişse
          {
              target->set_active(true); //pasif yap
          } else {
              target->set_active(false);
          }    
       
        if (target->type==PORT_FIRE) target->set_active(true); 
        if (target->type==PORT_EMERGENCY) target->set_active(true); 
        if (target->type==PORT_GAS) target->set_active(true); 
        if (target->type==PORT_WATER) target->set_active(true); 
        target->set_alarm(false);

        //printf("Port %15s ACTIVE=%d USER=%d\n",target->name,target->get_active(),target->get_user_active());
        target = target->next;
    }  
    write_status();
    //if (function_callback!=NULL) function_callback((void *)this, get_status());
}


//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Security::set_status(home_status_t stat)
{      
    bool chg=false;  
    if (!genel.virtual_device)
    {     
        if (stat.status==(int)SEC_STOP)
        {alarm_stop();} 
        else 
        {
        local_set_status(stat);
        if (status.status==(int)SEC_CLOSED)
        {
            tim_stop();
            alarm_close();
            strcpy((char*)status.ircom,""); 
            chg=true;
        }
        if (status.status==(int)SEC_PROCESS)
        {
            tim_stop();
            status.status = (int)SEC_CLOSED;
            chg=true;
        }
        if (status.status==(int)SEC_OPENED)
        {
            tim_start();
            strcpy((char*)status.ircom,""); //ircommand üzerinden port adı
            status.color = 0;
            status.status = (int)SEC_PROCESS;            
            chg=true;
        }  
        if (status.status==(int)SEC_STOP)
        {
            //status.status = (int)SEC_ALARM;
            alarm_stop(); 
            chg=true;
        }
        if (chg) {
            write_status();
            if (function_callback!=NULL) function_callback((void *)this, get_status());
        } 
        }
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }

}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Security::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=genel.active) chg=true;
    if (status.status!=stat.status) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(SEC_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}
void Security::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    cJSON_AddNumberToObject(obj, "status", stt.status);
    if (stt.status==4) 
    {
        cJSON_AddNumberToObject(obj, "color", stt.color);
        cJSON_AddStringToObject(obj, "ircom", (char*)stt.ircom);
    }
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Security::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Security::get_port_json(cJSON* obj)
{
    cJSON *point = cJSON_CreateArray();
    bool eklendi=false;
    Base_Port *target = port_head_handle;
    while(target)
    {
        cJSON *prt = cJSON_CreateObject();
        cJSON_AddStringToObject(prt,"name",target->name);
        cJSON_AddNumberToObject(prt, "id", target->id);
        cJSON_AddNumberToObject(prt, "type", (uint8_t)target->type);
        cJSON_AddItemToArray(point,prt);
        target = target->next;
        eklendi = true;
    }
    if (eklendi) cJSON_AddItemToObject(obj,"port",point); else cJSON_Delete(point); 
    return eklendi;
}

void Security::get_port_status_json(cJSON* obj)
{
    cJSON *point = cJSON_CreateArray();
    bool eklendi=false;
    Base_Port *target = port_head_handle;
    while(target)
    {
        cJSON *prt = cJSON_CreateObject();
        cJSON_AddNumberToObject(prt,"id",target->id);
        if (target->get_user_active()) cJSON_AddTrueToObject(prt, "act"); else cJSON_AddFalseToObject(prt, "act");
        if (target->type != PORT_OUTPORT)
         {
          if (target->get_alarm()) cJSON_AddTrueToObject(prt, "stat"); else cJSON_AddFalseToObject(prt, "stat");
         }
        if (target->type == PORT_OUTPORT) 
          {
          if (target->get_hardware_status()) cJSON_AddTrueToObject(prt, "stat"); else cJSON_AddFalseToObject(prt, "stat");
          }
        cJSON_AddItemToArray(point,prt);
        target = target->next;
        eklendi = true;
    }
    if (eklendi) cJSON_AddItemToObject(obj,"port",point); else cJSON_Delete(point); 
    //return eklendi;
}

//yangın bildirisi aldığında ne yapacak
void Security::fire(bool stat)
{
}

void Security::tim_callback(void* arg)
{   
    Security *aa = (Security *)arg;
    aa->alarm_open();
}

void Security::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Security::tim_start(void){
    if (qtimer!=NULL) {
      timer=10;  
      ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
      printf("timer=%d\n",timer);
    }
}

void Security::init(void)
{
  if (!genel.virtual_device)
  {  
    esp_timer_create_args_t arg = {};
    arg.callback = &tim_callback;
    arg.name = "Ltim0";
    arg.arg = (void *) this;
    ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
    if (timer==0) timer=10;
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type==PORT_OUTPORT)
            {
                status.stat = target->get_hardware_status();
                break;
            } else {
                //target->set_alarm(false);
                target->set_alarm((bool)target->get_hardware_status());
            }

        target=target->next;
    }
  }
}

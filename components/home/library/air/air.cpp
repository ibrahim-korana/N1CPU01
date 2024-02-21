#include "air.h"
#include "esp_log.h"

static const char *AIR_TAG = "AIR";

//Bu fonksiyon inporttan tetiklenir
void Air::func_callback(void *arg, port_action_t action)
{ }

void Air::temp_action(bool send)
{
    bool change = false, durum = false;
    if (status.temp>=status.set_temp) {
                                durum=false;
                                change=true;
                              } else {
                                       durum = true;
                                       change = true;
                                     }
    if (!status.stat) {durum=false;change=true;} 
    if (!status.active) {durum=false;change=true;} 
    if (change)
    {                                
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) status.status = target->set_status(durum);
            target = target->next;
        }
    }  
    if (send) if (function_callback!=NULL) function_callback((void *)this, get_status());
}


void Air::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        status.stat = stat.stat;
        status.active = stat.active;
        if (stat.set_temp!=status.set_temp ) 
        {
            status.set_temp = stat.set_temp;
            //burada set alt tarafa bildirilecek
            send_gateway=true;
        } 
        write_status();
        temp_action(false);
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

void Air::set_sensor(char *name, home_status_t stat)
{
    if (!genel.virtual_device)
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_VIRTUAL) 
                {
                    if (strcmp(target->name,name)==0) {
                        //printf("VIR PORT set %.2f=%.2f %.2f=%.2f \n",stat.temp,status.temp, stat.set_temp,status.set_temp); 
                        if (status.temp!=stat.temp || status.set_temp!=stat.set_temp) {
                            status.temp = stat.temp;
                            status.set_temp = stat.set_temp;
                            temp_action();
                        }
                    } 
                }
            target = target->next;
        } 
    }
}


//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Air::remote_set_status(home_status_t stat, bool callback_call) {
    
    bool chg = false;
    if (status.active!=stat.active) chg=true;
    if (status.stat!=stat.stat) chg=true;
    if (status.set_temp!=stat.set_temp) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(AIR_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      } 
           
}

void Air::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
    char *mm = (char *)malloc(10);
    sprintf(mm,"%2.02f",stt.temp); 
    cJSON_AddNumberToObject(obj, "temp", atof(mm));
    sprintf(mm,"%2.02f",stt.set_temp);
    cJSON_AddNumberToObject(obj, "stemp", atof(mm));
    free(mm);
}

void Air::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Air::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Air::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //ısıtma yangın ihbarında kapatılır
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.status = target->set_status(false);
                }
            target = target->next;
        }
    } else {
       status = main_temp_status;
       Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.status);
                }
            target = target->next;
        }      
    }
}

void Air::senaryo(char *par)
{
    cJSON *rcv = cJSON_Parse(par);
    if (rcv!=NULL)
    {
        JSON_getfloat(rcv,"temp",&(status.temp));
        JSON_getbool(rcv,"stat",&(status.stat));
        set_status(status);
        cJSON_Delete(rcv);  
    } 
}

void Air::init(void)
{
    if (!genel.virtual_device)
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type==PORT_OUTPORT)
                {
                    status.status = target->get_hardware_status();
                    break;
                }
            target=target->next;
        }
    }
}

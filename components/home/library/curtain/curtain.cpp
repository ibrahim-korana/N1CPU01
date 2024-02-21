

#include "curtain.h"
#include "esp_log.h"

static const char *CUR_TAG = "CURTAIN";

//Bu fonksiyon inporttan tetiklenir
void Curtain::func_callback(void *arg, port_action_t action)
{   
    if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Curtain *fnc = (Curtain *) arg;
        home_status_t stat = fnc->get_status();
        bool change = false;

        printf("Event %d\n",evt);

        if (evt==BUTTON_LONG_PRESS_START) 
          {
              fnc->buton_durumu = CURBTN_LONG;
          }
        if (evt==BUTTON_PRESS_DOWN)
          {
            if (fnc->Motor!=MOTOR_STOP)
                {
                //motor calışıyor. motoru ve timerı durdur
                fnc->Motor=MOTOR_STOP;
                fnc->tim_stop();
                stat.status = (int)CUR_HALF_CLOSED;
                change=true;
                fnc->temp_status = CUR_UNKNOWN;
                fnc->motor_action();
                } else {
                //motor stop durumda. motoru çalıştırıp zamanı başlat
                fnc->temp_status = (cur_status_t) fnc->status.status;
                if (strcmp(prt->name,"UP")==0) fnc->Motor=MOTOR_UP;
                if (strcmp(prt->name,"DOWN")==0) fnc->Motor=MOTOR_DOWN;
                stat.status = (int) CUR_PROCESS;
                change=true;
                fnc->tim_stop();
                fnc->tim_start();
                fnc->motor_action();
                }
          }
          if (evt==BUTTON_PRESS_UP) 
          {
            if (fnc->buton_durumu==CURBTN_LONG || fnc->eskitip) {
                fnc->temp_status = CUR_UNKNOWN;
                fnc->buton_durumu = CURBTN_UNKNOWN;
                //motoru ve timer ı durdur
                stat.status = (int) CUR_HALF_CLOSED;
                change=true;
                fnc->Motor=MOTOR_STOP;
                fnc->tim_stop();
                fnc->motor_action();
            }; 
          }
       if (change) fnc->local_set_status(stat,true);
     }
}

void Curtain::tim_stop(void){
  if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Curtain::tim_start(void){
  if (status.counter==0 || status.counter>60) status.counter=10;
  ESP_ERROR_CHECK(esp_timer_start_once(qtimer, status.counter * 1700000));
}

const char* Curtain::convert_motor_status(uint8_t mot)
{
  if (mot==MOTOR_STOP) return "STOP";
  if (mot==MOTOR_UP) return "UP";
  if (mot==MOTOR_DOWN) return "DOWN";
  return "UNKNOWN";
}
void Curtain::motor_action(bool callback) 

{  /*
    0 stop
    1 up
    2 down
    */
  Base_Port *target = port_head_handle;
  while (target) { 
    if (Motor==MOTOR_STOP) 
    {
        if (target->type==PORT_OUTPORT) target->set_status(false);
    }
    if (Motor==MOTOR_UP) 
    {
        if (target->type==PORT_OUTPORT && strcmp(target->name,"UP")==0) target->set_status(true);
    }
    if (Motor==MOTOR_DOWN) 
    {
        if (target->type==PORT_OUTPORT && strcmp(target->name,"DOWN")==0) target->set_status(true);
    }  
    target = target->next;
  }

  ESP_LOGI(CUR_TAG,"Motor Status : %s",convert_motor_status(Motor));
  if(function_callback!=NULL && callback) function_callback((void *)this,status);
}

void Curtain::cur_tim_callback(void* arg)
{
    Curtain *mthis = (Curtain *)arg;
    //Base_Function *base = (Base_Function *)arg;
    home_status_t st = mthis->get_status();
    ESP_LOGI(CUR_TAG,"Bekleme zamanı doldu");
    if (mthis->temp_status!=CUR_UNKNOWN)
    {   
        ESP_LOGI(CUR_TAG,"MOTOR Durduruluyor..");
        st.status = (int) mthis->temp_status;
        mthis->local_set_status(st,true);   
        mthis->temp_status = CUR_UNKNOWN;
        mthis->Motor= MOTOR_STOP;
        mthis->motor_action(); 
    }
}

void Curtain::init(void)
{
  
  if (!genel.virtual_device)
  {
    status.stat = true;
    status.status = CUR_CLOSED;
    esp_timer_create_args_t arg = {};
    arg.callback = &cur_tim_callback;
    arg.name = "tim0";
    arg.arg = (void *) this;
    ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
    Motor= MOTOR_STOP;
    motor_action(false);
    if (status.counter==0 || status.counter>60) status.counter=10;
  }
}

void Curtain::set_status(home_status_t stat)
{
  if (!genel.virtual_device)
    {
      local_set_status(stat,true);
      if (temp_status == CUR_UNKNOWN) {
          if (status.status==0 || status.status==2)
            {     tim_stop();
                  tim_start();
                  temp_status = (cur_status_t) status.status;
                  status.status = (int) CUR_PROCESS;
                  if (temp_status == CUR_CLOSED) Motor = MOTOR_DOWN;
                  if (temp_status == CUR_OPENED) Motor = MOTOR_UP;
                  motor_action(false);
            }
      } else {
          status.status = (int)CUR_HALF_CLOSED;
          tim_stop();
          temp_status = CUR_UNKNOWN;
          Motor= MOTOR_STOP;
          motor_action(false);
      }
      write_status();
      if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

void Curtain::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.status!=stat.status) chg=true;
    if (status.active!=genel.active) chg=true;
    if (status.counter!=stat.counter) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(CUR_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Curtain::ConvertStatus(home_status_t stt, cJSON *obj)
{
    cJSON_AddNumberToObject(obj, "status", stt.status);
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Curtain::get_status_json(cJSON* obj)
{
    return ConvertStatus(status, obj);
}

bool Curtain::get_port_json(cJSON* obj)
{
  return false;
}

void Curtain::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        status.status = CUR_OPENED;
        Motor = MOTOR_UP;
        motor_action();
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void Curtain::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.status = CUR_OPENED;
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.status = CUR_CLOSED;
        set_status(status);
      }  
}


void Curtain::role_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_OUTPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  } 
}

void Curtain::anahtar_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_INPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  }  
}


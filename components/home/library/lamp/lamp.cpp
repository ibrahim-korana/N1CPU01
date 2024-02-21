
#include "lamp.h"
#include "esp_log.h"

static const char *LAMP_TAG = "LAMP";

//Bu fonksiyon inporttan tetiklenir
void Lamp::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Base_Function *lamba = (Base_Function *) arg;
        home_status_t stat = lamba->get_status();
        bool change = false;
        if (lamba->genel.active)
        {           
            if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
            {
                //portları tarayıp çıkış portlarını bul
                Base_Port *target = lamba->port_head_handle;
                while (target) {
                    if (target->type == PORT_OUTPORT) 
                    {
                        if (prt->type==PORT_INPORT) {
                            if (evt==BUTTON_PRESS_DOWN) {stat.stat = target->set_status(true);change = true;((Lamp *)lamba)->tim_stop();}
                            if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_status(false);change = true;((Lamp *)lamba)->tim_stop();}
                        }
                        if (prt->type==PORT_INPULS) {
                            if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_toggle();change = true;((Lamp *)lamba)->tim_stop();}
                        }
                        if (prt->type==PORT_SENSOR) {
                            if (evt==BUTTON_PRESS_DOWN)
                            {
                                if (prt->status==true) {
                                    //lamba kapalı lambayı açıp timer çalıştır
                                    stat.stat = target->set_status(true);
                                    change = true;
                                    ((Lamp *)lamba)->tim_stop();
                                    ((Lamp *)lamba)->tim_start();
                                }
                            }
                        }
                    }       
                    target = target->next;
                                }
            }
        }
        if (change) {
            stat.counter = 0;
            ((Lamp *)lamba)->xtim_stop();
            ((Lamp *)lamba)->local_set_status(stat,true);
            if (lamba->function_callback!=NULL) lamba->function_callback(lamba, lamba->get_status());
        } 
     }
}


//bu fonksiyon lambayı yazılım ile tetiklemek için kullanılır.
void Lamp::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        if (!status.stat) {xtim_stop(); status.counter=0;}
        
        if (status.counter>0) {
           if (status.stat) xtim_start();
        }
        
        if (!genel.active) status.stat = false;
               
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.stat = target->set_status(status.stat);
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
void Lamp::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (genel.active!=stat.active) chg=true;
    if (status.counter!=stat.counter) chg=true;

    //printf("before active %d stat active %d chg %d\n",genel.active,stat.active, chg);

    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(LAMP_TAG,"%d Status Changed",genel.device_id);
         //printf("after active %d\n",genel.active);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Lamp::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter>0) cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Lamp::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Lamp::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Lamp::fire(bool stat)
{
    xtim_stop();
    status.counter=0;
    if (stat) {
        main_temp_status = status;
        //globali 1 tanımlı lamba acil lambasıdır
        //yangında aktif hale getir.
        //Bu durum lamba start edilmemiş bile olsa geçerlidir.
        //Bunun dışındaki lambalar kapatılır.
        if ((global & 0x01)!=0x01)
        {
           status.stat = false;
        } else {
           status.stat = true; 
        }
        set_status(status);
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void Lamp::senaryo(char *par)
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

void Lamp::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Lamp::tim_start(void){
    if (qtimer!=NULL)
      ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
}
void Lamp::xtim_stop(void){
    if (xtimer!=NULL)
      if (esp_timer_is_active(xtimer)) esp_timer_stop(xtimer);
}
void Lamp::xtim_start(void){
    if (xtimer!=NULL)
     if (!esp_timer_is_active(xtimer))
       ESP_ERROR_CHECK(esp_timer_start_periodic(xtimer, 60 * 1000000));
}

void Lamp::xtim_callback(void* arg)
{   
    //lamba/ları kapat
    Lamp *aa = (Lamp *) arg;
    home_status_t stat = aa->get_status();
    bool change=false;
    if (stat.counter>0) {stat.counter--;change=true;}
    if (stat.counter==0)
    {
        aa->xtim_stop();
        Base_Port *target = aa->port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    stat.stat = target->set_status(false);
                    change=true;
                }
            target = target->next;
        }
    }    
    if (change) aa->local_set_status(stat,true);
    if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
}


void Lamp::lamp_tim_callback(void* arg)
{   
    //lamba/ları kapat
    Base_Function *aa = (Base_Function *) arg;
    home_status_t stat = aa->get_status();
    Base_Port *target = aa->port_head_handle;
    bool change=false;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                stat.stat = target->set_status(false);
                change=true;
            }
        target = target->next;
    }
    if (change) aa->local_set_status(stat,true);
    if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
}

void Lamp::init(void)
{
    status.counter = 0;
    if (!genel.virtual_device)
    { 
        esp_timer_create_args_t arg1 = {};
        arg1.callback = &xtim_callback;
        arg1.name = "Ltim1";
        arg1.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg1, &xtimer)); 
        disk.read_status(&status,genel.device_id);
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type==PORT_OUTPORT)
                {
                status.stat = target->get_hardware_status();
                break;
                }
            target=target->next;
        }

        //printf("ID %d HW Stat=%d\n",genel.device_id,status.stat);

        if ((global & 0x02) == 0x02) 
        {
            //Karşılama lambasıdır Timerı tanımla boşta beklesin 
            esp_timer_create_args_t arg = {};
            arg.callback = &lamp_tim_callback;
            arg.name = "Ltim0";
            arg.arg = (void *) this;
            ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
            if (timer==0) timer=30;
        }
    }
}

/*
   Fonksiyonun json tanımı ve özellikleri
        {
            "name":"lamp",
            "id": 1,      
            "loc": 0,
            "subdev":0,          
            "timer": 10,
            "global": 2,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 1, "pcf":1, "name":"anahtar", "type":"PORT_INPORT"},
                    {"pin": 4, "pcf":1, "name":"sensor", "type":"PORT_SENSOR"},
                    {"pin": 1, "pcf":1, "name":"role", "type":"PORT_OUTPORT"}
                ]
            }
        }

    name : "lamp" olmak zorundadır.
    id   : tanım id sidir. Bu numara sistem tarafından verilir.
    loc  : fonksiyonun fiziki yerini tanımlar. (kullanılmıyor)
    subdev: (kullanılmıyor)
    timer : sensor tanımı varsa lambanın ne kadar yanık kalacagını belirtir. 
            0 verilirse default 30 alınır.
    global : bit bazında bir tanımlamadır. 
                ---- --XX
             En sağdaki X lambanın yangın ikaz lambası olduğunu gösterir.
             ikinci X ise karşılama lambası olduğunu gösterir. 
             Buraya verilebilecek değerler:
                1  Yangın ikaz lambası
                2  Karşılama lambası
                3  Yangın ikaz ve karşılama lambası

              Yangın ikaz lambası : Yangın alarmı alındığında bu lamba sönükse yakılır ve ana 
                                    kontaktör kapanana kadar yanık kalır.
              Karşılama lambası : Sensör girişi aktif olduğunda lamba yakılır ve tanımlanan 
                                  süre sonunda söndürülür. Bekleme süresinde lamba manuel 
                                  olarak veya program aracılığı ile söndürülebilir.  
               
 
    hardware: Fiziki tanımların yapıldığı bölümdür
        location : fiziki transmisyonu tanımlar. (Sistem kendisi ayarlar.)
        port : fonsksiyon için gerekli in/out birimlerinin tanımlandığı bölümdür
               lamba için bir çıkış portu tanımlamak zorunludur. Diğer portlar isteğe bağlıdır.
               tanımlanabilecek portlar;
                   1. PORT_INPORT
                   2. PORT_SENSOR
                   3. PORT_OUTPORT
                   4. PORT_INPULS
            pin : portun fiziki pin numarasıdır. Eğer port pcf üzerinde ise
                  PORT_INPORT için 1-20, PORT_OUTPORT için 1-12 olmalıdır. Eğer 
                  port CPU üzerinde ise CPU gpio numarası tanımlanmalıdır. 
            pcf : portun pcf veya cpu üzerinde olduğunu gösterir. pcf:0 tanımı
                  portun cpu üzerinde pcf:1 tanımı pcf üzerinde olduğunu söyler
            name : Portun ismidir. lamba için önemsizdir.  
            type : portun tipini gösterir. PORT_INPORT,PORT_OUTPORT vb.
            reverse : INPORT tipi portlar eğer cpu üzerinde ise aksiyonun 
                      hangi kenarda gerçekleşecegini tanımlar. Default aksiyon 
                      0 dır. Eğer porta 1 geldiğinde aksiyon almasını istiyorsanız
                      reverse:1 tanımını yapınız.                          

    Lamba için birden çok giriş veya birden çok çıkış portu tanımlanabilir. 
      -   Birden çok çıkış portu tanımlayarak gurup lamba  oluşturabilirsiniz. 
          (tek anahtara baglı çoklu lamba. bahçe aydınlarması vb)
      -   INPULS olmak kaydı ile birden çok anahtar bağlayarak vaviyen bağlantı sağlanabilir. 
      -   SENSOR portları kullanılarak alarm sistemlerinde otomatik aydınlatma 
          sağlanabilir. (kapı sensörü ile karşılama lambası, PIR ile güvenlik aydınlatması vb)  
      -   Hedef :
              - LDR sensör ile ortam ışığına bağlı On/Off
              - Zamana bağlı On/Off
              

    Lamba set_status fonksiyonu ile;
        1. stat=true, stat=false ile açılıp kapatılabilir. 
        2. active=true/false ile iptal edilebilir. 
        3. counter>0 özelliği ile belirlenen süre kadar açık bırakılır.
         
*/
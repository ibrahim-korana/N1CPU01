
#ifndef _CLASSES_H
#define _CLASSES_H

#include "iot_out.h"
#include "iot_button.h"
#include "core.h"
#include "storage.h"
#include "cJSON.h"
#include "esp_timer.h"

static const char *CLASSES_TAG = "CLASSES";

class Base_Port 
{
    public:
       Base_Port();
       ~Base_Port() {
           free(name);
       };
       void set_outport(out_handle_t ou) {outport = ou;}
       void set_inport(button_handle_t inp) {inport = inp;}

        bool set_port_type(port_type_t tp, void *usr_data);
        void set_active(bool dr);
        void set_active_admin(bool dr);
        bool get_active(void) {return active;}
        void set_user_active(bool dr);
        bool get_user_active(void) {return user_active;}
        bool set_status(bool st);
        bool set_toggle(void);
        bool get_hardware_status();
        bool get_alarm(void) {return alarm;}
        void set_alarm(bool al) {alarm=al;}
        void set_callback(void* bas, port_callback_t cb) {
          base_function=bas; 
          port_action_callback = cb;
        }

        char *name;
        uint8_t id=0;
        port_type_t type;
        bool status;
        bool virtual_port=false;
        void *base_function;
       
       out_handle_t outport = NULL;
       button_handle_t inport = NULL;

       uint8_t Klemens;
   /*
       inports_t inport={};
       port_action_t action = {};

       virtual bool get_status(void) {return status;};
       */
        Base_Port *next=NULL; 

       protected:
        /*
           in port'da istenen aksiyon oluştuğunda lokal olarak bu fonksiyon çağırılır
           bu callback de gerekli düzenlemeleri yaparak port_action_callback üzerinden
           aksiyonu dışarıya aktarır
        */
        static void inportscall(void *arg, void *usr); 
        /*
            out portta bir aksiyon  oluştuğunda lokal olarak bu fonksiyon çağırılır 
            bu callback de gerekli düzenlemeleri yaparak port_action_callback üzerinden
            aksiyonu dışarıya aktarır
        */
        static void outportscall(void *arg, void *usr);  
         /*active portun işlevini yerine getirmesini veya tamamen kapanmasını sağlar*/ 
         bool active = true;
         /*
         User  Active portun kullanıcı tarafından iptal edilmesini sağlar.
         User Active true olmayan bir port start/stop olamaz.
         */
         bool user_active = true;
         /*
         Portta aksiyon oluştuğunda bu fonksiyon çağrılır. 
         böylece oluşan degişim fonksiyona aktarılır. Bu callback portun baglı oldugu
         fonksiyon tarafından set_callback çağırılarak oluşturulur.
         */
         port_callback_t port_action_callback=NULL;
         bool alarm = false;
};


static void reg_tim_callback(void* arg);

class Base_Function
{
    public:
      function_prop_t genel;
      uint8_t statustype;

      uint8_t timer;
      uint8_t global;
      uint8_t prg_loc;
      /*
          fonksiyon function_start_register çagrıldığında bir süre bekler sonrasında 
          eğer tanımlanmışsa register_callback callbacki çağırır. Bu fonksiyon
          ana cihaza fonksiyonun özelliklerini göndererek register kaydı ister.
          ana cihazdan register cevabı gelince function_set_register çağrılarak
          çağrı tekrarı durdurulur ve register bilgileri kaydedilir. Sistemin
          amacı fonksiyonu ana cihaza register etmektir. 
      */
      register_callback_t register_callback = NULL;
      /*
          fonksiyonda oluşan tüm değişimlerde ilgili yerlere haber vermek üzere
          (program veya diğer fonksiyonlar) bu callback çağırılır. bu callback
          ana program tarafından konulur. 
      */
      function_callback_t function_callback = NULL;
      /*
          Base_fonction nesensini devralan sınıflar local_port_callback
          tanımlayarak input ve output aksiyonlarını alabilirler.  
      */
      port_callback_t local_port_callback = NULL;
      /*
          Eger fonksiyon virtual olarak tanımlanmış ise gelen değişim
          emirlerini alt veya üst device a aktarabilmek için bu callback
          kullanılır. 
      */
      function_callback_t command_callback = NULL;
      

      
      Base_Function() {
        //genel.name = (char *) calloc(1,15);
        genel.location = TR_NONE;
        global = 0;
        timer = 0;
        prg_loc = 0;
        genel.register_id = 0;
        genel.register_device_id = 0;
        genel.virtual_device=false;
        genel.registered=false;
        genel.active = true;
        esp_timer_create_args_t arg = {};
        arg.callback = &reg_tim_callback;
        arg.name = "tim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &reg_tim));
         
      };
      ~Base_Function() {
        //free(name);
      };

      home_status_t main_temp_status={};
      Base_Port *port_head_handle = NULL;
      uint8_t port_count=0;

      bool get_active(void) {return genel.active;}
      
      void add_port(Base_Port *dev, bool debug=false)
      {
          dev->next = port_head_handle;
          port_head_handle = dev;
          dev->id = ++port_count;
          dev->set_callback((void *)this,local_port_callback);
      } ;  

      void list_port(void) {
        Base_Port *target = port_head_handle;
        //ESP_LOGI(CLASSES_TAG,"%3d %-15s",genel.device_id,genel.name);
        while (target) {
          char *aa= (char *)malloc(30);
          port_type_string(target->type,aa);
          ESP_LOGW(CLASSES_TAG,"                   >> %-15s %-15s Klemens %d",target->name,aa,target->Klemens);
          free(aa);
          target = target->next;
        }
        //ESP_LOGI(CLASSES_TAG,"-----------------------------------");
      }  

      virtual void set_status(home_status_t stat) {};
      virtual void remote_set_status(home_status_t stat, bool callback_call=true) {};
      
      virtual void get_status_json(cJSON* obj) = 0; //tüm objeler bunu tanımlamak zorunda
      virtual bool get_port_json(cJSON* obj) = 0;
      virtual void get_port_status_json(cJSON* obj) {};
      
      
      virtual void init(void) {};
      virtual void fire(bool stat) {}; 
      virtual void water(bool stat) {}; 
      virtual void gas(bool stat) {}; 
      virtual void set_sensor(char *name, home_status_t stat) {}; 
      virtual void senaryo(char *par) {};
      

      virtual void ConvertStatus(home_status_t stt, cJSON* obj) {};

      void function_start_register(void) {
        genel.register_id = 0;
        genel.registered = false;
        if(esp_timer_is_active(reg_tim)) esp_timer_stop(reg_tim);
        ESP_ERROR_CHECK(esp_timer_start_periodic(reg_tim,  genel.device_id * 1000000)); 
      };
   
      void function_set_register(uint8_t id, uint8_t dev) {
        if(esp_timer_is_active(reg_tim)) esp_timer_stop(reg_tim);
        genel.register_id = id;
        genel.register_device_id = dev;
        genel.registered = true;
      }

      void set_active(bool drm)
      {
          genel.active = drm;
          Base_Port *target = port_head_handle;
          while (target) {
              target->set_active(genel.active);
              target = target->next;
          }
      }

      void write_status(void)
      {   
          disk.write_status(&status, genel.device_id);
      }

      home_status_t get_status(void)
      {
        return status;
      }; 

      void local_set_status(home_status_t stat, bool write=false)
      {
          memcpy(&status,&stat,sizeof(home_status_t));
          //status=stat;
          if (write) write_status(); 
      }

      Base_Function *next = NULL;

      protected:
         //Türetilenden erişilir   
         home_status_t status;
         Storage disk;
      private:
          esp_timer_handle_t reg_tim;

};

static void reg_tim_callback(void* arg)
{
    Base_Function *mthis = (Base_Function *)arg;
    if (mthis->register_callback!=NULL) mthis->register_callback(arg);
}




#endif
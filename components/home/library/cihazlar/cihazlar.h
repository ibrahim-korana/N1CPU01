#ifndef __CIHAZLAR_H__
#define __CIHAZLAR_H__

#include "core.h"
#include "storage.h"
#include "lwip/err.h"
#include "lwip/ip.h"
#include "string.h"
#include "esp_log.h"
#include <lwip/etharp.h>

#include "../rs485/rs485.h"


class Cihazlar 
{
    public:
        Cihazlar(){};
        ~Cihazlar(){};
        device_register_t *get_handle(void) {return dev_handle;}
        device_register_t *cihaz_ekle(const char *mac,transmisyon_t trns);

        esp_err_t cihaz_sil(const char *mac);
        uint8_t cihaz_say(void);
        esp_err_t cihaz_list(void);
        esp_err_t cihaz_bosalt(void);
        esp_err_t update_ip(const char *mac,uint32_t ip);
        device_register_t *cihazbul(char *mac);
        device_register_t *cihazbul(uint8_t id);
        device_register_t *cihazbul_soket(uint8_t soket);
        eth_addr *get_sta_mac(const uint32_t &ip, char *mac);
        void start(bool st) {status = st;}

        void init(RS485 *rs)
          {
            rs485 = rs;
            xTaskCreate(garbage, "garbage", 2048, (void *) this, 5,NULL);
          }

        RS485 *rs485; 
        bool status=false;

    private:
        device_register_t *dev_handle = NULL;   
        device_register_t *_cihaz_sil(device_register_t *deletedNode,const char *mac);  
        static void garbage(void *arg);

};




#endif
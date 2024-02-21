#ifndef __LIST_H__
#define __LIST_H__

#include "lwip/err.h"
#include "string.h"
#include "esp_log.h"
#include "iptool.h"

#define CMAC_LEN 6

class List 
{
    public:
        List(){Adr = IPAddr();}
        ~List(){};
        void *get_handle(void) {return dev_handle;}
        now_client_t *cihaz_ekle(const uint8_t *mac,uint8_t id);
        esp_err_t cihaz_sil(const char *mac);
        uint8_t cihaz_say(void);
        esp_err_t cihaz_list(void);
        esp_err_t cihaz_bosalt(void);
        now_client_t *cihazbul(char *mac);
        now_client_t *cihazbul(uint8_t id);
        
    private:
        now_client_t *dev_handle = NULL;   
        now_client_t *_cihaz_sil(now_client_t *deletedNode,const char *mac); 
        IPAddr Adr; 
};




#endif
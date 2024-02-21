#ifndef _IPTOOL_H
#define _IPTOOL_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/socket.h"
#include "netdb.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

class IPAddr {
  public:
     IPAddr(){buf = (char*)malloc(14);}
     ~IPAddr(){free(buf);buf=NULL;};
     char *to_string(uint32_t a0) {
        adr.addr = a0;
        strcpy(buf,ip4addr_ntoa(&adr));
        return buf;
     }
     uint32_t to_int(const char*a0) {
        if (ip4addr_aton(a0,&adr)) return adr.addr;
        return 0;
     }
     char *mac_to_string(const uint8_t *mc) {
         sprintf(buf,"%02X%02X%02X%02X%02X%02X",mc[0],mc[1],mc[2],mc[3],mc[4],mc[5]);
         return buf;
     }

  private:
     ip4_addr_t adr;  

     char *buf; 
};


#endif

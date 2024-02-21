

#include "core.h"
#include "string.h"

port_type_t port_type_convert(char * aa)
{    
  if (strcmp(aa,"PORT_NC")==0) return PORT_NC;
  if (strcmp(aa,"PORT_INPORT")==0) return PORT_INPORT;
  if (strcmp(aa,"PORT_INPULS")==0) return PORT_INPULS;
  if (strcmp(aa,"PORT_OUTPORT")==0) return PORT_OUTPORT;
  if (strcmp(aa,"PORT_FIRE")==0) return PORT_FIRE;
  if (strcmp(aa,"PORT_WATER")==0) return PORT_WATER;
  if (strcmp(aa,"PORT_GAS")==0) return PORT_GAS;
  if (strcmp(aa,"PORT_EMERGENCY")==0) return PORT_EMERGENCY;
  if (strcmp(aa,"PORT_SENSOR")==0) return PORT_SENSOR;
  if (strcmp(aa,"PORT_VIRTUAL")==0) return PORT_VIRTUAL;  
  return PORT_NC;
}

void port_type_string(port_type_t tp,char *aa)
{
    if (aa!=NULL)
      {
        switch(tp) {
            case PORT_NC : {strcpy(aa,"PORT_NC");break;}
            case PORT_INPORT : {strcpy(aa,"PORT_INPORT");break;}
            case PORT_INPULS : {strcpy(aa,"PORT_INPULS");break;}
            case PORT_OUTPORT : {strcpy(aa,"PORT_OUTPORT");break;}
            case PORT_FIRE : {strcpy(aa,"PORT_FIRE");break;}
            case PORT_WATER : {strcpy(aa,"PORT_WATER");break;}
            case PORT_GAS : {strcpy(aa,"PORT_GAS");break;}
            case PORT_EMERGENCY : {strcpy(aa,"PORT_EMERGENCY");break;}
            case PORT_SENSOR : {strcpy(aa,"PORT_SENSOR");break;}
            case PORT_VIRTUAL : {strcpy(aa,"PORT_VIRTUAL");break;}
            default : {strcpy(aa,"Error");break;}
        }
      }
}

void transmisyon_string(transmisyon_t tt,char *aa)
{
  if (aa!=NULL)
      {
        switch(tt) {
            case TR_NONE : {strcpy(aa,"TR_NONE");break;}
            case TR_UDP : {strcpy(aa,"TR_UDP");break;}
            case TR_SERIAL : {strcpy(aa,"TR_SERIAL");break;}
            case TR_ESPNOW : {strcpy(aa,"TR_ESPNOW");break;}
            case TR_LOCAL : {strcpy(aa,"TR_LOCAL");break;}
            default : {strcpy(aa,"Error");break;}
        }
      }
}

void transmisyon_type_convert(home_wifi_type_t tp,char *aa)
{
      if (aa!=NULL)
      {
        switch(tp) {
            case HOME_WIFI_AP : {strcpy(aa,"Access Point");break;}
            case HOME_WIFI_STA : {strcpy(aa,"Wifi Station");break;}
            case HOME_WIFI_AUTO : {strcpy(aa,"Wifi Auto");break;}
            case HOME_ETHERNET : {strcpy(aa,"Ethernet");break;}
            case HOME_NETWORK_DISABLE : {strcpy(aa,"Network Kapalı");break;}
            default : {strcpy(aa,"Error");break;}
        }
      } 
}
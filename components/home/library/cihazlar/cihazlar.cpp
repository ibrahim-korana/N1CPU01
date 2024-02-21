
#include "cihazlar.h"

#include "../wifi_now/wifi_now.h"


const char *CHZTAG ="CIHARLAR";

void Cihazlar::garbage(void *arg)
{
    Cihazlar *self = (Cihazlar *) arg;
    while(1)
     {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        if (self->status)
          {
              device_register_t *target =self->get_handle();
              while(target)
              {
                  bool ret=false, png=false;
                  if (target->transmisyon==TR_SERIAL) {png=true;ret = self->rs485->ping(target->device_id);}
                  if (target->transmisyon==TR_ESPNOW && target->device_id!=255) {
                    png=true;
                    ret= EspNOW_ping(target->device_id);
                  }
                  if (png) 
                   {
                    if (ret)
                        {
                            target->active=1;
                            target->oldactive=0;
                        } else {
                          target->active=0;
                          target->oldactive++;
                          }
                    vTaskDelay(100/portTICK_PERIOD_MS);
                   }
                  target=(device_register_t *)target->next;         
              }
          }    
     }
     vTaskDelete(NULL);
}

eth_addr *Cihazlar::get_sta_mac(const uint32_t &ip, char *mac)
{
    //gelen ipnin mac adresini dondurur
    ip4_addr requestIP{ip};
    eth_addr *ret_eth_addr = nullptr;
    ip4_addr const *ret_ip_addr = nullptr;
    if (etharp_request(netif_default, &requestIP)==ESP_OK)
    {
    if (etharp_find_addr(netif_default, &requestIP, &ret_eth_addr, &ret_ip_addr)>-1)
    {
    sprintf(mac,"%02X%02X%02X%02X%02X%02X",
    ret_eth_addr->addr[0],
    ret_eth_addr->addr[1],
    ret_eth_addr->addr[2],
    ret_eth_addr->addr[3],
    ret_eth_addr->addr[4],
    ret_eth_addr->addr[5]);
    return ret_eth_addr;
    }
    }
    return NULL;
}

device_register_t *Cihazlar::cihazbul(char *mac)
{
  //printf("Aranan MAC %s\n",mac);
   device_register_t *target = dev_handle;
   while(target)
   {
      if (strcmp(mac,target->mac)==0) return target;
      target=(device_register_t *)target->next;
   }
   return NULL;
}
device_register_t *Cihazlar::cihazbul(uint8_t id)
{
   device_register_t *target = dev_handle;
   while(target)
   {
      if (id==target->device_id) {return target;}
      target=(device_register_t *)target->next;
   }
   return NULL;
}

device_register_t *Cihazlar::cihazbul_soket(uint8_t soket)
{
  //printf("Aranan MAC %s\n",mac);
   device_register_t *target = dev_handle;
   while(target)
   {
      if (soket==target->socket) return target;
      target=(device_register_t *)target->next;
   }
   return NULL;
}


device_register_t *Cihazlar::cihaz_ekle(const char *mac,transmisyon_t trns)
{
  device_register_t *target = dev_handle;
  while(target)
    {
      if (strcmp(mac,target->mac)==0) {target->transmisyon= trns; return target;} 
      target=(device_register_t *)target->next;
    }
  device_register_t *yeni = (device_register_t *) calloc(1, sizeof(device_register_t));
  ESP_ERROR_CHECK(NULL == yeni);
  memset(yeni,0,sizeof(device_register_t));
      
  strcpy(yeni->mac,mac);
  yeni->transmisyon= trns;
  yeni->next = dev_handle;
  yeni->socket = -1;
  dev_handle = yeni;
  return yeni;
}

device_register_t *Cihazlar::_cihaz_sil(device_register_t *deletedNode,const char *mac)
{
	device_register_t *temp;
	device_register_t *iter = deletedNode;
	
  if (deletedNode==NULL) return NULL;
	if(strcmp(mac,deletedNode->mac)==0)
  {
		temp = deletedNode;
		deletedNode = (device_register_t *)deletedNode->next;
		free(temp); // This is the function delete item
		return deletedNode;
	}
	
	while(iter->next != NULL && strcmp(mac,((device_register_t *)(iter->next))->mac)!=0){
		iter = (device_register_t *)iter->next;
	}
	
	if(iter->next == NULL){
		return deletedNode;
	}
	temp = (device_register_t *)iter->next;
	iter->next = (device_register_t *)((device_register_t *)(iter->next))->next;
	free(temp);
	return deletedNode;
}

esp_err_t Cihazlar::cihaz_sil(const char *mac)
{
  dev_handle = _cihaz_sil(dev_handle, mac);
  return ESP_OK; 

}

uint8_t Cihazlar::cihaz_say(void)
{
  device_register_t *target = dev_handle;
  uint8_t sy = 0;
  while(target) {sy++;target=(device_register_t *)target->next;}
  return sy;
}

esp_err_t Cihazlar::cihaz_list(void)
{
  device_register_t *target = dev_handle;
  uint8_t sy = 0;
    ESP_LOGI(CHZTAG, "          %3s %-9s %3s %3s %3s %-12s %-16s %4s","ID","TRN","ACT","ACT","COU","MAC","IP","SOCK");
    ESP_LOGI(CHZTAG, "          --- --------- --- ----------------------------------");
    char *mm = (char*)malloc(10);
    char *ww = (char*)malloc(20);
    while (target)
      {
        transmisyon_string(target->transmisyon,mm);
        //printf("IP %d\n",target->ip);
        ip_addr_t rr; rr=IPADDR4_INIT(target->ip);
        strcpy(ww,ipaddr_ntoa(&rr));
        ESP_LOGI(CHZTAG, "          %3d %-9s %3d %3d %3d %-12s %-16s %4d",
                             target->device_id,
                             mm,
                             target->active,
                             target->oldactive,
                             target->function_count,
                             target->mac,
                             ww,
                             target->socket
                             );
        target=(device_register_t *)target->next;
      }
      free(mm);
      free(ww);
  return sy;
}

esp_err_t Cihazlar::update_ip(const char *mac,uint32_t ip)
{
   device_register_t *target = dev_handle;
   while(target)
   {
      if (strcmp(mac,target->mac)==0) {target->ip=ip;return ESP_OK;}
      target = (device_register_t *)target->next;
   }
   return ESP_FAIL;
}

esp_err_t Cihazlar::cihaz_bosalt(void)
{
  device_register_t *target = dev_handle;
  device_register_t *tmp;
  while(target)
   {
        tmp = target;
        target = (device_register_t *)target->next;
        free(tmp);tmp=NULL;
   }
   dev_handle=target;
   return ESP_OK;
}

#include "list.h"


const char *LISTTAG ="LIST";


now_client_t *List::cihazbul(char *mac)
{
  //printf("Aranan MAC %s\n",mac);
   now_client_t *target = dev_handle;
   while(target)
   {
      if (strcmp(mac,target->mac)==0) return target;
      target=(now_client_t *)target->next;
   }
   return NULL;
}
now_client_t *List::cihazbul(uint8_t id)
{
   now_client_t *target = dev_handle;
   while(target)
   {
      if (id==target->device_id) {return target;}
      target=(now_client_t *)target->next;
   }
   return NULL;
}

now_client_t *List::cihaz_ekle(const uint8_t *mac,uint8_t id )
{
  char *mm = (char *)malloc((CMAC_LEN*2)+1);
  strcpy(mm,Adr.mac_to_string(mac));

  now_client_t *target = dev_handle;
  while(target)
    {
      if (strcmp(mm,target->mac)==0) {target->device_id= id; free(mm);return target;} 
      target=(now_client_t *)target->next;
    }
  now_client_t *yeni = (now_client_t *) calloc(1, sizeof(now_client_t));
  ESP_ERROR_CHECK(NULL == yeni);
  memset(yeni,0,sizeof(now_client_t));
  memcpy(yeni->mac_0,mac,CMAC_LEN);   
  strcpy(yeni->mac,mm);
  yeni->device_id= id;
  yeni->next = dev_handle;
  dev_handle = yeni;
  free(mm);
  return yeni;
}

now_client_t *List::_cihaz_sil(now_client_t *deletedNode,const char *mac)
{
	now_client_t *temp;
	now_client_t *iter = deletedNode;
	
  if (deletedNode==NULL) return NULL;
	if(strcmp(mac,deletedNode->mac)==0)
  {
		temp = deletedNode;
		deletedNode = (now_client_t *)deletedNode->next;
		free(temp); // This is the function delete item
		return deletedNode;
	}
	
	while(iter->next != NULL && strcmp(mac,((now_client_t *)(iter->next))->mac)!=0){
		iter = (now_client_t *)iter->next;
	}
	
	if(iter->next == NULL){
		return deletedNode;
	}
	temp = (now_client_t *)iter->next;
	iter->next = (now_client_t *)((now_client_t *)(iter->next))->next;
	free(temp);
	return deletedNode;
}

esp_err_t List::cihaz_sil(const char *mac)
{
  dev_handle = _cihaz_sil(dev_handle, mac);
  return ESP_OK; 

}

uint8_t List::cihaz_say(void)
{
  now_client_t *target = dev_handle;
  uint8_t sy = 0;
  while(target) {sy++;target=(now_client_t *)target->next;}
  return sy;
}

esp_err_t List::cihaz_list(void)
{
  now_client_t *target = dev_handle;
  uint8_t sy = 0;
    ESP_LOGI(LISTTAG, "          %3s %-12s","ID","MAC");
    ESP_LOGI(LISTTAG, "          --- ------------");
    char *mm = (char*)malloc(10);
    char *ww = (char*)malloc(20);
    while (target)
      {
        ESP_LOGI(LISTTAG, "          %3d %-12s",
                             target->device_id,
                             target->mac
                             );
        target=(now_client_t *)target->next;
      }
      free(mm);
      free(ww);
  return sy;
}

esp_err_t List::cihaz_bosalt(void)
{
  now_client_t *target = dev_handle;
  now_client_t *tmp;
  while(target)
   {
        tmp = target;
        target = (now_client_t *)target->next;
        free(tmp);tmp=NULL;
   }
   dev_handle=target;
   return ESP_OK;
}
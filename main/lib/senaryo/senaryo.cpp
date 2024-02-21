
#include "senaryo.h"

const char *FUNTAG = "SENARYO";

senaryo_t *senaryo_handle = NULL;

senaryo_t *get_senaryo_handle(void)
{
   return senaryo_handle;
}

void add_senaryo(senaryo_t *sn)
{
   sn->next = senaryo_handle;
   senaryo_handle = sn;
}

uint8_t count_senaryo(void)
{
    uint8_t ret = 0;
    senaryo_t *tmp = senaryo_handle;
    while(tmp)
    {
        ret++;
        tmp=(senaryo_t *)tmp->next;
    }
    return ret;
}

void clear_senaryo(void)
{  
   senaryo_t *tmp = senaryo_handle;
    while(tmp)
    {
        senaryo_t *next = (senaryo_t *)tmp->next;
        if (tmp!=NULL) {
          
          free(tmp->name);tmp->name=NULL;
          free(tmp->act);tmp->act=NULL;
          free(tmp->pas);tmp->pas=NULL;         
          free(tmp);tmp=NULL;
        }
        tmp=next;
    }
    senaryo_handle = NULL;
}

void get_sen_time(cJSON *tm,uint8_t *s0, uint8_t *d0,uint8_t *s1, uint8_t *d1, days_t *dy)
{
  cJSON *tim=NULL;
  cJSON *start=NULL;
  cJSON *stop=NULL;
  cJSON *days=NULL;
  cJSON *item=NULL;

  if (JSON_item_control(tm, "time")) {
    tim = cJSON_GetObjectItemCaseSensitive(tm, "time");
    if (JSON_item_control(tim, "start")) {
      start = cJSON_GetObjectItemCaseSensitive(tim, "start");
      JSON_getint(start,"hour",s0);
      JSON_getint(start,"minute",d0);
    }
    if (JSON_item_control(tim, "stop")) {
      stop = cJSON_GetObjectItemCaseSensitive(tim, "stop");
      JSON_getint(stop,"hour",s1);
      JSON_getint(stop,"minute",d1);
    }
    if (JSON_item_control(tim, "days")) {
       days = cJSON_GetObjectItem(tim,"days");
       uint8_t kk=0;
       cJSON_ArrayForEach(item, days) 
       {         
          if (kk==0) dy->pzr = item->valueint;
          if (kk==1) dy->pzt = item->valueint;
          if (kk==2) dy->sal = item->valueint;
          if (kk==3) dy->crs = item->valueint;
          if (kk==4) dy->per = item->valueint;
          if (kk==5) dy->cum = item->valueint;
          if (kk==6) dy->cmt = item->valueint; 
          if (kk==7) dy->all = item->valueint; 
          kk++;
       }
    }
  }  
}

void *get_sen_read(cJSON *tm)
{
  cJSON *func=NULL;
  cJSON *item=NULL;

  if (JSON_item_control(tm, "functions")) {
       func = cJSON_GetObjectItem(tm,"functions");
       cJSON_ArrayForEach(item, func) 
       { 
          senaryo_t *sen = (senaryo_t *)malloc(sizeof(senaryo_t));
          if (sen==NULL) {ESP_LOGE(FUNTAG, "sen memory not allogate"); return NULL;}

          uint8_t uzn = JSON_getstringlen(item,"name");
          if (uzn>0) {
            sen->name = (char *)malloc(uzn+1);
            if (sen->name==NULL) {ESP_LOGE(FUNTAG, "sen->name memory not allogate"); return NULL;}
            JSON_getstring(item,"name",sen->name);
          }

          uzn = JSON_getstringlen(item,"act");
          if (uzn>0) {
            sen->act = (char *)malloc(uzn+1);
            if (sen->act==NULL) {ESP_LOGE(FUNTAG, "sen->act memory not allogate"); return NULL;}
            JSON_getstring(item,"act",sen->act);
          }

          uzn = JSON_getstringlen(item,"pas");
          if (uzn>0) {
            sen->pas = (char *)malloc(uzn+1);
            if (sen->pas==NULL) {ESP_LOGE(FUNTAG, "sen->pas memory not allogate"); return NULL;}
            JSON_getstring(item,"pas",sen->pas);
          }
          add_senaryo(sen);
          //printf("%s %s %s\n",sen->name,sen->act,sen->pas); 
       }

  }
  return NULL;
}

void *senaryo_read(uint8_t senno, TimeTrigger *trg, TimeTrigger_callback_t start_cb, TimeTrigger_callback_t stop_cb,Storage dsk)
{
   clear_senaryo();
   char *fname1 = (char *)malloc(40);
   if (fname1==NULL) {ESP_LOGE(FUNTAG, "fname memory not allogate"); return NULL;}
   
   strcpy(fname1,"/config/reset.json");
   if (dsk.file_search(fname1))
     {
        uint8_t *sn = (uint8_t *) malloc(sizeof(uint8_t));
        *sn = 99;       
        int fsize = dsk.file_size(fname1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(fname1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",fname1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        //printf("reset senaryo buyuklugu %d\n",fsize);
        if (fsize>256) {ESP_LOGE(FUNTAG, "Senaryo çok büyük : %d<256",fsize); return NULL;}
        cJSON *rcv_json = cJSON_Parse(buf);
        free(buf);
        buf=NULL;
        uint8_t a_saat, a_dak, b_saat, b_dak;
        days_t *dy = (days_t *)malloc(sizeof(days_t));
        get_sen_time(rcv_json, &a_saat, &a_dak, &b_saat, &b_dak, dy);
        //printf("Reset %d:%d %d %d\n",a_saat,a_dak, dy->cmt, dy->all);
        cJSON_Delete(rcv_json);
        trg->set_callback(a_saat,a_dak,dy,start_cb,(void *)sn);
        ESP_LOGI(FUNTAG,"Reset Senaryosu Eklendi");
     }

     if (senno>0)
     {
        sprintf(fname1,"/config/senaryo%d.json",senno);
        if (dsk.file_search(fname1))
            {
                uint8_t *sn = (uint8_t *) malloc(sizeof(uint8_t));
                uint8_t *sn1 = (uint8_t *) malloc(sizeof(uint8_t));
                *sn = senno;
                *sn1= senno;
                int fsize = dsk.file_size(fname1); 
                //printf("senaryo buyuklugu %d\n",fsize);
                if (fsize>1500) {ESP_LOGE(FUNTAG, "Senaryo çok büyük : %d<1500",fsize); return NULL;}
                char *buf = (char *) malloc(fsize+5);
                if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
                FILE *fd = fopen(fname1, "r");
                if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",fname1); return NULL;}
                fread(buf, fsize, 1, fd);
                fclose(fd);                
                cJSON *rcv_json = cJSON_Parse(buf);
                free(buf);
                buf=NULL;
                uint8_t a_saat, a_dak, b_saat, b_dak;
                days_t *dy = (days_t *)malloc(sizeof(days_t));
                days_t *dy1 = (days_t *)malloc(sizeof(days_t));
                get_sen_time(rcv_json, &a_saat, &a_dak, &b_saat, &b_dak, dy);
                memcpy(dy1,dy,sizeof(days_t));
                trg->set_callback(a_saat,a_dak,dy,start_cb,(void *)sn);
                trg->set_callback(b_saat,b_dak,dy1,stop_cb,(void *)sn1);
                //printf("Sen %d:%d %d:%d %d %d\n",a_saat,a_dak, b_saat,b_dak,dy->cmt, dy->all);
                get_sen_read(rcv_json);
                cJSON_Delete(rcv_json);                
                ESP_LOGI(FUNTAG,"Senaryo %d Eklendi",senno);
            }   
     }
    
  free(fname1);  
  //fname1=NULL; 
  return NULL;   
}
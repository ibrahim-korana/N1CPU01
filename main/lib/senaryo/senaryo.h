#ifndef _SENARYO_H
#define _SENARYO_H

   #include "htime.h"
   #include "storage.h"
   #include "esp_log.h"
   #include "jsontool.h"

   typedef struct {
    char *name;
    char *act;
    char *pas; 
    void *next;
} senaryo_t;


   
   void *senaryo_read(uint8_t senno, TimeTrigger *trg, 
                      TimeTrigger_callback_t start_cb, 
                      TimeTrigger_callback_t stop_cb,
                      Storage dsk);
   void clear_senaryo(void);
   uint8_t count_senaryo(void);
   void add_senaryo(senaryo_t sn);
   senaryo_t *get_senaryo_handle(void);


#endif
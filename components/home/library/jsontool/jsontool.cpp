

#include "cJSON.h"
#include "string.h"
#include "../core/core.h"

bool JSON_item_control(cJSON *rt,const char *item)
{
    if (cJSON_GetObjectItem(rt, item)) return true;
    else return false;
}

bool JSON_getstring(cJSON *rt,const char *item, char *value)
{
    if (JSON_item_control(rt, item)) {
              strcpy(value , (char *)cJSON_GetObjectItem(rt,item)->valuestring);
              return true; 
	                                 } 
    else return false;
}

uint8_t JSON_getstringlen(cJSON *rt,const char *item)
{
    if (JSON_item_control(rt, item)) {
        cJSON *nm = cJSON_GetObjectItem(rt,item);
        if (nm!=NULL) return strlen(nm->valuestring);
        return 0;
    }
    return 0;
}

bool JSON_getint(cJSON *rt,const char *item, uint8_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valueint;
        return true;
    }
    return false;
}

bool JSON_getbool(cJSON *rt,const char *item, bool *value)
{
    if (JSON_item_control(rt, item)) {
        *value = (bool *)cJSON_GetObjectItem(rt,item)->valueint;
        return true;
    }
    return false;
}

bool JSON_getfloat(cJSON *rt,const char *item, float *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valuedouble;
        return true;
    }
    return false;
}

bool JSON_getlong(cJSON *rt,const char *item, uint64_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valuedouble;
        return true;
    }
    return false;
}

void json_to_status(cJSON *rt, home_status_t *stat)
{
   if (JSON_item_control(rt,"durum")) 
        {
            cJSON *fmt = cJSON_GetObjectItem(rt,"durum"); 
            /*          
            if (JSON_item_control(fmt,"stat")) stat->stat = cJSON_GetObjectItem(fmt,"stat")->valueint;
            if (JSON_item_control(fmt,"temp")) stat->temp = cJSON_GetObjectItem(fmt,"temp")->valuedouble;
            if (JSON_item_control(fmt,"stemp")) stat->set_temp = cJSON_GetObjectItem(fmt,"stemp")->valuedouble;
            if (JSON_item_control(fmt,"color")) stat->color = (uint64_t)cJSON_GetObjectItem(fmt,"color")->valuedouble; 
            if (JSON_item_control(fmt,"status")) stat->status = cJSON_GetObjectItem(fmt,"status")->valueint;
            if (JSON_item_control(fmt,"ircom")) strcpy((char*)stat->ircom,cJSON_GetObjectItem(fmt,"ircom")->valuestring);
            if (JSON_item_control(fmt,"irval")) strcpy((char*)stat->irval,cJSON_GetObjectItem(fmt,"irval")->valuestring);
            if (JSON_item_control(fmt,"coun")) stat->counter = cJSON_GetObjectItem(fmt,"coun")->valueint;
            if (JSON_item_control(fmt,"act")) stat->active = cJSON_GetObjectItem(fmt,"act")->valueint;
            */
            JSON_getbool(fmt,"stat",&(stat->stat));
            JSON_getfloat(fmt,"temp",&(stat->temp));
            JSON_getfloat(fmt,"stemp",&(stat->set_temp));
            JSON_getlong(fmt,"color",(uint64_t *)&(stat->color));
            JSON_getint(fmt,"status",&(stat->status));
            JSON_getstring(fmt,"ircom",(char *)&(stat->ircom));
            JSON_getstring(fmt,"irval",(char *)&(stat->irval));
            JSON_getint(fmt,"coun",&(stat->counter));
            JSON_getbool(fmt,"act",&(stat->active));


        }
}

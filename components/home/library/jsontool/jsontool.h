#ifndef _JSON_TOOL_H_
#define _JSON_TOOL_H_

  #include "cJSON.h"

  bool JSON_item_control(cJSON *rt,const char *item);
  bool JSON_getstring(cJSON *rt,const char *item, char *value);
  uint8_t JSON_getstringlen(cJSON *rt,const char *item);
  bool JSON_getint(cJSON *rt,const char *item, uint8_t *value);
  bool JSON_getbool(cJSON *rt,const char *item, bool *value);
  bool JSON_getfloat(cJSON *rt,const char *item, float *value);
  bool JSON_getlong(cJSON *rt,const char *item, uint64_t *value);

  void json_to_status(cJSON *rt, home_status_t *stat);

#endif
#ifndef _STORAGE_H
#define _STORAGE_H

#include "../core/core.h"


#define FORMAT_SPIFFS_IF_FAILED true

class Storage {
    public:
      Storage() {};
      ~Storage(){};

      bool init(void);
      bool format(void);

      bool file_search(const char *name);
      bool file_empty(const char *name);
      bool file_control(const char *name);
      int file_size(const char *name);
      bool file_create(const char *name, uint16_t size);
      //void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
      
      bool write_status(home_status_t *stat, uint8_t obj_num, bool start=false);
      bool read_status(home_status_t *stat, uint8_t obj_num);
      bool write_file(const char *name, void *flg, uint16_t size, uint8_t obj_num);
      bool read_file(const char *name, void *flg, uint16_t size, uint8_t obj_num);

      bool function_file_format(void);
      bool status_file_format(void);
      
      void file_format(void);

      static const char *rangematch(const char *pattern, char test, int flags);
      static int fnmatch(const char *pattern, const char *string, int flags);
      static void list(const char *path, const char *match);
/*
      bool write_dev_config(remote_reg_t *stat, uint8_t obj_num, bool start=false);
      bool read_dev_config(remote_reg_t *stat, uint8_t obj_num);
*/   


};

#endif
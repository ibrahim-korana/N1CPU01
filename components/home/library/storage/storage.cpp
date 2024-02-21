

#include "storage.h"
#include "esp_spiffs.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "../core/core.h"
#include <errno.h>
#include <sys/time.h>
#include "esp_vfs.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <ctype.h>

#define STATUS_FILE "/config/status.bin"
#define FUNCTION_FILE "/config/function.bin"

const char *TAG="STORAGE";

bool Storage::format(void)
{
   esp_spiffs_format("storage");
   return true;
}

bool Storage::function_file_format(void)
{
    FILE *fd = fopen(FUNCTION_FILE, "r+");
    if (fd == NULL) {ESP_LOGE(TAG,"%s not open",FUNCTION_FILE); return false;}
    fseek(fd, 0, SEEK_SET);  
    char *ff = (char *)malloc(sizeof(function_reg_t)*MAX_DEVICE);
    memset(ff, 0,sizeof(function_reg_t)*MAX_DEVICE);
    fwrite(ff,1,sizeof(function_reg_t)*MAX_DEVICE,fd);
    free(ff);
    fflush(fd);
    fclose(fd);
    return true; 
}

bool Storage::status_file_format(void)
{
    FILE *fd = fopen(STATUS_FILE, "r+");
    if (fd == NULL) {ESP_LOGE(TAG,"%s not open",STATUS_FILE); return false;}
    fseek(fd, 0, SEEK_SET);  
    char *ff = (char *)malloc(sizeof(home_status_t)*MAX_FUNCTION);
    memset(ff, 0,sizeof(home_status_t)*MAX_FUNCTION);
    fwrite(ff,1,sizeof(home_status_t)*MAX_FUNCTION,fd);
    free(ff);
    fflush(fd);
    fclose(fd);
    return true; 

}

void Storage::file_format(void)
{
         home_status_t stat0 = {};
         printf("%s Formating\n",STATUS_FILE);
         write_status(&stat0,0,true);
          /*
         printf("%s Formating\n",DEVICE_FILE);
         remote_reg_t stat2 = {};
         stat2.device_id = 1;
         stat2.register_id = 1;
         strcpy(stat2.name,"bos kayit");
         stat2.register_device_id = 0;
         stat2.yer = 0;
         write_dev_config(&stat2,0,true);
         */
}

bool Storage::init(void)
{   bool err=true;
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/config",
      .partition_label = "storage",
      .max_files = 8,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
         
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    if (!file_control(STATUS_FILE)) return false;
   
    if (!file_control(FUNCTION_FILE)) return false;
    if (file_size(FUNCTION_FILE)==0) function_file_format();
    if (file_size(STATUS_FILE)==0) status_file_format();

    return err;
}

//diskte dosya varsa true döner
bool Storage::file_search(const char *name)
{
  struct stat st;  
  if (stat(name, &st) != 0) {
        ESP_LOGE(TAG, "%s not FOUND",name);
        return false;
  } else { 
    return true; 
  }   
  return false;
}

//diskte dosya varsa içini boşaltır.
bool Storage::file_empty(const char *name)
{
  if (file_search(name)) 
  {
    FILE *fd = fopen(name, "w");
    if (fd == NULL) {
      ESP_LOGE(TAG, "%s not created",name);
      return false;
    }
    fclose(fd);
    return true;
  } 
  return false;
}

bool Storage::file_create(const char *name, uint16_t size)
{
    FILE *fd = fopen(name, "w");
    if (fd == NULL) {
      ESP_LOGE(TAG, "%s not created",name);
      return false;
    }
    fseek(fd, (1*size), SEEK_SET);
    char *ff = (char *)malloc(size*MAX_DEVICE);
    memset(ff, 0,size*MAX_DEVICE);
    fwrite(ff,1,size*MAX_DEVICE,fd);
    free(ff);
    fclose(fd);
    return true;
}

//diskte dosya yoksa boş olarak oluşturur.
bool Storage::file_control(const char *name)
{
    if (!file_search(name)) {
        FILE *fd = fopen(name, "w");
        if (fd==NULL) {ESP_LOGE(TAG,"%s not created",name);return false;}
        fclose(fd);
        return true;
    }                      
  return true;
}

int Storage::file_size(const char *name)
{
    FILE *fd = fopen(name, "r+");
    if (fd == NULL) {ESP_LOGE(TAG, "%s not open",name); return 0;}
    fseek(fd, 0, SEEK_END);
    int file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    fclose(fd);
    return file_size;
}


bool Storage::write_status(home_status_t *stat, uint8_t obj_num, bool start)
{
     FILE *fd = fopen(STATUS_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG,"%s not open",STATUS_FILE); return false;}
     fseek(fd, (obj_num*sizeof(home_status_t)), SEEK_SET);
     if (start) {
        char *ff = (char *)malloc(sizeof(home_status_t)*MAX_DEVICE);
        memset(ff, 0,sizeof(home_status_t)*MAX_DEVICE);
        fwrite(ff,1,sizeof(home_status_t)*MAX_DEVICE,fd);
        free(ff);
     } else fwrite(stat,1,sizeof(home_status_t),fd);
     fflush(fd);
     fclose(fd);
     return true; 
}
bool Storage::read_status(home_status_t *stat, uint8_t obj_num)
{
     FILE *fd = fopen(STATUS_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG, "%s not open",STATUS_FILE); return false;}
     fseek(fd, (obj_num*sizeof(home_status_t)), SEEK_SET);
     fread(stat,1,sizeof(home_status_t),fd);
     fclose(fd);
     return true; 
}

bool Storage::write_file(const char *name, void *flg, uint16_t size, uint8_t obj_num)
{
     FILE *fd = fopen(name, "r+");
     if (fd == NULL) {ESP_LOGE(TAG, "%s not open",name); return false;}
     fseek(fd, (obj_num*size), SEEK_SET);
     fwrite(flg,1,size,fd);
     fflush(fd);
     fclose(fd);
     return true; 
}
bool Storage::read_file(const char *name, void *flg, uint16_t size, uint8_t obj_num)
{
     FILE *fd = fopen(name, "r+");
     if (fd == NULL) {ESP_LOGE(TAG, "%s not open",name); return false;}
     fseek(fd, (obj_num*size), SEEK_SET); 
     fread(flg,1,size,fd);
     fclose(fd);
     return true;
}

/*
bool Storage::write_byte(uint8_t *stat, uint8_t obj_num, bool start)
{
     FILE *fd = fopen(BYTE_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG,"WRITE >> %s not open",BYTE_FILE); return false;}
     fseek(fd, (obj_num*sizeof(uint8_t)), SEEK_SET);
     if (start) {
        char *ff = (char *)malloc(sizeof(uint8_t)*100);
        memset(ff, 0,sizeof(uint8_t)*100);
        fwrite(ff,1,sizeof(uint8_t)*100,fd);
        free(ff);
     } else fwrite(stat,1,sizeof(uint8_t),fd);
     fflush(fd);
     fclose(fd);
     return true; 
}
bool Storage::read_byte(uint8_t *stat, uint8_t obj_num)
{
     FILE *fd = fopen(BYTE_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG, "READ >> %s not open",BYTE_FILE); return false;}
     fseek(fd, (obj_num*sizeof(uint8_t)), SEEK_SET);
     fread(stat,1,sizeof(uint8_t),fd);
     fclose(fd);
     return true; 
}

bool Storage::write_dev_config(remote_reg_t *stat, uint8_t obj_num, bool start)
{
     FILE *fd = fopen(DEVICE_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG,"%s not open",DEVICE_FILE); return false;}
    
     uint16_t offset = obj_num*sizeof(remote_reg_t);  
     fseek(fd, offset, SEEK_SET);
     if (start) {
        char *ff = (char *)malloc(sizeof(remote_reg_t)*100);
        memset(ff, 0,sizeof(remote_reg_t)*100);
        fwrite(ff,1,sizeof(remote_reg_t)*100,fd);
        free(ff);
     } else {
        fwrite(stat,1,sizeof(remote_reg_t),fd);
     }
     fflush(fd);
     fclose(fd);
     return true; 
}
bool Storage::read_dev_config(remote_reg_t *stat, uint8_t obj_num)
{
     FILE *fd = fopen(DEVICE_FILE, "r+");
     if (fd == NULL) {ESP_LOGE(TAG, "%s not open",DEVICE_FILE); return false;}
     uint16_t offset = obj_num*sizeof(remote_reg_t);
     fseek(fd, offset, SEEK_SET);
     fread(stat,1,sizeof(remote_reg_t),fd);
     fclose(fd);
     return true; 
}

*/

#define	FNM_NOMATCH     1       // Match failed.
#define	FNM_NOESCAPE	0x01	// Disable backslash escaping.
#define	FNM_PATHNAME	0x02	// Slash must be matched by slash.
#define	FNM_PERIOD		0x04	// Period must be matched by period.
#define	FNM_LEADING_DIR	0x08	// Ignore /<tail> after Imatch.
#define	FNM_CASEFOLD	0x10	// Case insensitive search.
#define  FNM_PREFIX_DIRS	0x20	// Directory prefixes of pattern match too.
#define	EOS	            '\0'

//-----------------------------------------------------------------------
const char *Storage::rangematch(const char *pattern, char test, int flags)
{
  int negate, ok;
  char c, c2;

  /*
   * A bracket expression starting with an unquoted circumflex
   * character produces unspecified results (IEEE 1003.2-1992,
   * 3.13.2).  This implementation treats it like '!', for
   * consistency with the regular expression syntax.
   * J.T. Conklin (conklin@ngai.kaleida.com)
   */
  if ( (negate = (*pattern == '!' || *pattern == '^')) ) ++pattern;

  if (flags & FNM_CASEFOLD) test = tolower((unsigned char)test);

  for (ok = 0; (c = *pattern++) != ']';) {
    if (c == '\\' && !(flags & FNM_NOESCAPE)) c = *pattern++;
    if (c == EOS) return (NULL);

    if (flags & FNM_CASEFOLD) c = tolower((unsigned char)c);

    if (*pattern == '-' && (c2 = *(pattern+1)) != EOS && c2 != ']') {
      pattern += 2;
      if (c2 == '\\' && !(flags & FNM_NOESCAPE)) c2 = *pattern++;
      if (c2 == EOS) return (NULL);

      if (flags & FNM_CASEFOLD) c2 = tolower((unsigned char)c2);

      if ((unsigned char)c <= (unsigned char)test &&
          (unsigned char)test <= (unsigned char)c2) ok = 1;
    }
    else if (c == test) ok = 1;
  }
  return (ok == negate ? NULL : pattern);
}

//--------------------------------------------------------------------
int Storage::fnmatch(const char *pattern, const char *string, int flags)
{
  const char *stringstart;
  char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++) {
    case EOS:
      if ((flags & FNM_LEADING_DIR) && *string == '/') return (0);
      return (*string == EOS ? 0 : FNM_NOMATCH);
    case '?':
      if (*string == EOS) return (FNM_NOMATCH);
      if (*string == '/' && (flags & FNM_PATHNAME)) return (FNM_NOMATCH);
      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
          ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
              return (FNM_NOMATCH);
      ++string;
      break;
    case '*':
      c = *pattern;
      // Collapse multiple stars.
      while (c == '*') c = *++pattern;

      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
          ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
              return (FNM_NOMATCH);

      // Optimize for pattern with * at end or before /.
      if (c == EOS)
        if (flags & FNM_PATHNAME)
          return ((flags & FNM_LEADING_DIR) ||
                    strchr(string, '/') == NULL ?
                    0 : FNM_NOMATCH);
        else return (0);
      else if ((c == '/') && (flags & FNM_PATHNAME)) {
        if ((string = strchr(string, '/')) == NULL) return (FNM_NOMATCH);
        break;
      }

      // General case, use recursion.
      while ((test = *string) != EOS) {
        if (!fnmatch(pattern, string, flags & ~FNM_PERIOD)) return (0);
        if ((test == '/') && (flags & FNM_PATHNAME)) break;
        ++string;
      }
      return (FNM_NOMATCH);
    case '[':
      if (*string == EOS) return (FNM_NOMATCH);
      if ((*string == '/') && (flags & FNM_PATHNAME)) return (FNM_NOMATCH);
      if ((pattern = rangematch(pattern, *string, flags)) == NULL) return (FNM_NOMATCH);
      ++string;
      break;
    case '\\':
      if (!(flags & FNM_NOESCAPE)) {
        if ((c = *pattern++) == EOS) {
          c = '\\';
          --pattern;
        }
      }
      break;
      // FALLTHROUGH
    default:
      if (c == *string) {
      }
      else if ((flags & FNM_CASEFOLD) && (tolower((unsigned char)c) == tolower((unsigned char)*string))) {
      }
      else if ((flags & FNM_PREFIX_DIRS) && *string == EOS && ((c == '/' && string != stringstart) ||
    		  (string == stringstart+1 && *stringstart == '/')))
              return (0);
      else return (FNM_NOMATCH);
      string++;
      break;
    }
  // NOTREACHED
  return 0;
}

// ============================================================================

//-----------------------------------------
void Storage::list(const char *path, const char *match) {

    DIR *dir = NULL;
    struct dirent *ent;
    char type;
    char size[12];
    char tpath[255];
    char tbuffer[80];
    struct stat sb;
    struct tm *tm_info;
    char *lpath = NULL;
    int statok;

    printf("\nList of Directory [%s]\n", path);
    printf("-----------------------------------\n");
    // Open directory
    dir = opendir(path);
    if (!dir) {
        printf("Error opening directory\n");
        return;
    }

    // Read directory entries
    uint64_t total = 0;
    int nfiles = 0;
    printf("T  Size      Date/Time         Name\n");
    printf("-----------------------------------\n");
    while ((ent = readdir(dir)) != NULL) {
        sprintf(tpath, path);
        if (path[strlen(path)-1] != '/') strcat(tpath,"/");
        strcat(tpath,ent->d_name);
        tbuffer[0] = '\0';

        if ((match == NULL) || (fnmatch(match, tpath, (FNM_PERIOD)) == 0)) {
            // Get file stat
            statok = stat(tpath, &sb);

            if (statok == 0) {
                tm_info = localtime(&sb.st_mtime);
                strftime(tbuffer, 80, "%d/%m/%Y %R", tm_info);
            }
            else sprintf(tbuffer, "                ");

            if (ent->d_type == DT_REG) {
                type = 'f';
                nfiles++;
                if (statok) strcpy(size, "       ?");
                else {
                    total += sb.st_size;
                    if (sb.st_size < (1024*1024)) sprintf(size,"%8d", (int)sb.st_size);
                    else if ((sb.st_size/1024) < (1024*1024)) sprintf(size,"%6dKB", (int)(sb.st_size / 1024));
                    else sprintf(size,"%6dMB", (int)(sb.st_size / (1024 * 1024)));
                }
            }
            else {
                type = 'd';
                strcpy(size, "       -");
            }

            printf("%c  %s  %s  %s\r\n",
                type,
                size,
                tbuffer,
                ent->d_name
            );
        }
    }
    if (total) {
        printf("-----------------------------------\n");
    	if (total < (1024*1024)) printf("   %8d", (int)total);
    	else if ((total/1024) < (1024*1024)) printf("   %6dKB", (int)(total / 1024));
    	else printf("   %6dMB", (int)(total / (1024 * 1024)));
    	printf(" in %d file(s)\n", nfiles);
    }
    printf("-----------------------------------\n");

    closedir(dir);

    free(lpath);
}
#ifndef __ping_h__
#define __ping_h__

#include "ping/ping_sock.h"

void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args);
void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args);
void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args);
esp_err_t initialize_ping(uint32_t interval_ms, uint32_t task_prio, char * target_host);
esp_err_t ping_system(char *ip, uint8_t ping_time);

void ping_stop(void);
void ping_start(void);

#endif /* __ping_h__ */

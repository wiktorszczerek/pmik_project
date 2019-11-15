#ifndef __WIFI_HANDLER_H
#define __WIFI_HANDLER_H
#include "freertos/event_groups.h"

void wifi_init_sta();
void wait_for_ip();
int get_got_ip();
void start_socket_client();
int send_data(char* data);

#endif

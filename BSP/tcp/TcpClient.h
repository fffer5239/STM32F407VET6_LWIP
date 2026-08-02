#ifndef _TCPCLIENT_H_
#define _TCPCLIENT_H_

#include <stdbool.h>
#include "lwip/tcp.h"

#define SERVER_IP "192.168.1.100"

#define TCP_CLIENT_PORT 1030

#define MAX_RECONNECT_ATTEMPTS 5

extern int g_reconnect_attempts;

void tcp_client_init(void);

bool tcp_client_is_connected(void);

void tcp_client_send_data(char* data);

#endif
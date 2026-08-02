#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "lwip/tcp.h"

#define TCP_SERVER_PORT        8888

#define MAX_TCP_CLIENTS        10

#define TCP_BUFFER_SIZE        1024

void tcp_server_init(void);

#endif
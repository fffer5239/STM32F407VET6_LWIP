#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <stdint.h>
#include "lwip/tcp.h"

#define TCP_SERVER_PORT        8888

#define MAX_TCP_CLIENTS        10

#define TCP_BUFFER_SIZE        1024

/**
  * @brief TCP Server 接收回调函数类型
  * @param  slot  来自哪个客户端槽位 (0 ~ MAX_TCP_CLIENTS-1)
  * @param  data  收到的原始字节（不含 \0 结尾，可能含 0x00）
  * @param  len   字节长度
  */
typedef void (*tcp_server_rx_cb_t)(int slot, const uint8_t *data, uint16_t len);

void tcp_server_init(void);

/* 注册接收回调，收到的原始字节会以 (slot, data, len) 形式交给回调处理 */
void tcp_server_set_rx_callback(tcp_server_rx_cb_t cb);

/* 向指定槽位的客户端发送字节，返回 0 成功 / -1 失败 */
int tcp_server_send_to_client(int slot, const uint8_t *data, uint16_t len);

/* 向所有已连接客户端广播字节 */
void tcp_server_broadcast(const uint8_t *data, uint16_t len);

#endif
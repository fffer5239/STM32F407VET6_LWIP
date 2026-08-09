#ifndef _TCPCLIENT_H_
#define _TCPCLIENT_H_

#include <stdbool.h>
#include <stdint.h>
#include "lwip/tcp.h"

#define SERVER_IP "192.168.1.100"

#define TCP_CLIENT_PORT 1030

#define MAX_RECONNECT_ATTEMPTS 5

#define TCP_RX_BUFFER_SIZE 1024

extern int g_reconnect_attempts;

/**
  * @brief TCP Client 接收回调函数类型
  * @param  data  收到的原始字节（不含 \0 结尾，可能含 0x00）
  * @param  len   字节长度
  */
typedef void (*tcp_client_rx_cb_t)(const uint8_t *data, uint16_t len);

void tcp_client_init(void);

bool tcp_client_is_connected(void);

/* 便捷接口：发送字符串（内部按 strlen 定长） */
void tcp_client_send_data(char* data);

/* 字节接口：发送任意长度的十六进制字节数组 */
void tcp_client_send_bytes(const uint8_t *data, uint16_t len);

/* 注册接收回调，收到的原始字节会以 (data, len) 形式交给回调处理 */
void tcp_client_set_rx_callback(tcp_client_rx_cb_t cb);

#endif
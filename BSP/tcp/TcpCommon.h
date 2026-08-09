#ifndef _TCP_COMMON_H_
#define _TCP_COMMON_H_

#include <stdint.h>

/**
  * @brief  将一段字节按 16 进制打印到串口，用于调试二进制收发
  * @param  tag   前缀标签，如 "TCP Client RX"
  * @param  data  字节数据指针
  * @param  len   字节长度
  * @note   每行 16 字节，格式: tag [len]\r\n  01 02 03 ...
  *         实现位于 TcpCommon.c
  */
void tcp_print_hex(const char *tag, const uint8_t *data, uint16_t len);

#endif /* _TCP_COMMON_H_ */

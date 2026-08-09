#include "TcpCommon.h"
#include <stdio.h>

void tcp_print_hex(const char *tag, const uint8_t *data, uint16_t len)
{
    printf("%s [%u]\r\n  ", tag, len);
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i & 0x0F) == 0x0F) {
            printf("\r\n  ");
        }
    }
    printf("\r\n");
}

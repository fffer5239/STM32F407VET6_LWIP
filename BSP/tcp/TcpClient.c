#include "TcpClient.h"
#include "TcpCommon.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static struct tcp_pcb *g_client_pcb = NULL;
static bool g_is_connected = false;
int g_reconnect_attempts = 0;

static tcp_client_rx_cb_t s_rx_cb = NULL;
static uint8_t s_rx_buf[TCP_RX_BUFFER_SIZE];

static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcp_client_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len);
static void tcp_client_err_callback(void *arg, err_t err);
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_poll_callback(void *arg, struct tcp_pcb *pcb);

bool tcp_client_is_connected(void)
{
    return g_is_connected;
}

static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (p == NULL) {
        printf("TCP Client: Connection closed by server\r\n");
        tcp_close(pcb);
        g_client_pcb = NULL;
        g_is_connected = false;
        return ERR_OK;
    }

    u16_t copy_len = p->tot_len > TCP_RX_BUFFER_SIZE ? TCP_RX_BUFFER_SIZE : p->tot_len;
    pbuf_copy_partial(p, s_rx_buf, copy_len, 0);
    if (p->tot_len > TCP_RX_BUFFER_SIZE) {
        printf("TCP Client: RX too large (%u), truncated to %u\r\n", p->tot_len, TCP_RX_BUFFER_SIZE);
    }

    tcp_print_hex("TCP Client RX", s_rx_buf, copy_len);

    if (s_rx_cb != NULL) {
        s_rx_cb(s_rx_buf, copy_len);
    }

    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_client_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    return ERR_OK;
}

static void tcp_client_err_callback(void *arg, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    printf("TCP Client: Error %d, connection lost\r\n", err);
    g_client_pcb = NULL;
    g_is_connected = false;
}

static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (err != ERR_OK) {
        printf("TCP Client: Connect failed, err=%d\r\n", err);
        g_client_pcb = NULL;
        g_is_connected = false;
        return err;
    }

    printf("TCP Client: Connected to server\r\n");
    g_is_connected = true;
    g_reconnect_attempts = 0;

    tcp_recv(pcb, tcp_client_recv_callback);
    tcp_sent(pcb, tcp_client_sent_callback);
    tcp_err(pcb, tcp_client_err_callback);
    tcp_poll(pcb, tcp_client_poll_callback, 4);

    return ERR_OK;
}

static err_t tcp_client_poll_callback(void *arg, struct tcp_pcb *pcb)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    return ERR_OK;
}

void tcp_client_init(void)
{
    if (g_client_pcb != NULL) {
        printf("TCP Client: Already initialized\r\n");
        return;
    }

    g_client_pcb = tcp_new();
    if (g_client_pcb == NULL) {
        printf("TCP Client: Failed to create PCB\r\n");
        return;
    }

    ip_addr_t server_ip;
    if (ipaddr_aton(SERVER_IP, &server_ip) != 1) {
        printf("TCP Client: Invalid server IP: %s\r\n", SERVER_IP);
        tcp_close(g_client_pcb);
        g_client_pcb = NULL;
        return;
    }

    tcp_arg(g_client_pcb, NULL);

    /* 必须在 connect 之前注册错误回调：
       否则首次连接失败时（PC 端监听未就绪 → RST/超时），lwIP 释放 pcb 后
       静默失败，g_client_pcb 变成悬空指针，主循环重连会被
       "Already initialized" 挡住，永远无法再连接 */
    tcp_err(g_client_pcb, tcp_client_err_callback);

    err_t err = tcp_connect(g_client_pcb, &server_ip, TCP_CLIENT_PORT, tcp_client_connected_callback);
    if (err != ERR_OK) {
        printf("TCP Client: tcp_connect failed, err=%d\r\n", err);
        tcp_close(g_client_pcb);
        g_client_pcb = NULL;
        g_is_connected = false;
    }
}

void tcp_client_send_bytes(const uint8_t *data, uint16_t len)
{
    if (!g_is_connected || g_client_pcb == NULL) {
        printf("TCP Client: Not connected, cannot send\r\n");
        return;
    }

    if (len == 0) {
        return;
    }

    err_t err = tcp_write(g_client_pcb, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("TCP Client: tcp_write failed, err=%d\r\n", err);
        return;
    }

    tcp_output(g_client_pcb);
    tcp_print_hex("TCP Client TX", data, len);
}

void tcp_client_send_data(char* data)
{
    tcp_client_send_bytes((const uint8_t *)data, (uint16_t)strlen(data));
}

void tcp_client_set_rx_callback(tcp_client_rx_cb_t cb)
{
    s_rx_cb = cb;
}
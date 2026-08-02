#include "TcpClient.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static struct tcp_pcb *g_client_pcb = NULL;
static bool g_is_connected = false;
int g_reconnect_attempts = 0;

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

    char buffer[256];
    u16_t copy_len = p->tot_len > (sizeof(buffer) - 1) ? (sizeof(buffer) - 1) : p->tot_len;
    pbuf_copy_partial(p, buffer, copy_len, 0);
    buffer[copy_len] = '\0';
    printf("TCP Client Recv: %s\r\n", buffer);

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

    err_t err = tcp_connect(g_client_pcb, &server_ip, TCP_CLIENT_PORT, tcp_client_connected_callback);
    if (err != ERR_OK) {
        printf("TCP Client: tcp_connect failed, err=%d\r\n", err);
        tcp_close(g_client_pcb);
        g_client_pcb = NULL;
        g_is_connected = false;
    }
}

void tcp_client_send_data(char* data)
{
    if (!g_is_connected || g_client_pcb == NULL) {
        printf("TCP Client: Not connected, cannot send\r\n");
        return;
    }

    u16_t len = (u16_t)strlen(data);
    err_t err = tcp_write(g_client_pcb, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("TCP Client: tcp_write failed, err=%d\r\n", err);
        return;
    }

    tcp_output(g_client_pcb);
    printf("TCP Client Sent: %s\r\n", data);
}
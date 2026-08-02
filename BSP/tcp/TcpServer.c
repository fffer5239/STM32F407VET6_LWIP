#include "TcpServer.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static struct tcp_pcb *g_server_pcb = NULL;
static struct tcp_pcb *g_client_pcbs[MAX_TCP_CLIENTS] = {NULL};

static err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcp_server_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len);
static void tcp_server_err_callback(void *arg, err_t err);
static err_t tcp_server_poll_callback(void *arg, struct tcp_pcb *pcb);

static int find_free_client_slot(void)
{
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (g_client_pcbs[i] == NULL) {
            return i;
        }
    }
    return -1;
}

static void remove_client(struct tcp_pcb *pcb)
{
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (g_client_pcbs[i] == pcb) {
            g_client_pcbs[i] = NULL;
            printf("TCP Server: Client %d removed\r\n", i);
            break;
        }
    }
}

static err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (err != ERR_OK) {
        printf("TCP Server: Accept error %d\r\n", err);
        return err;
    }

    int slot = find_free_client_slot();
    if (slot < 0) {
        printf("TCP Server: Max clients reached, rejecting\r\n");
        tcp_abort(newpcb);
        return ERR_MEM;
    }

    g_client_pcbs[slot] = newpcb;

    tcp_arg(newpcb, newpcb);
    tcp_recv(newpcb, tcp_server_recv_callback);
    tcp_sent(newpcb, tcp_server_sent_callback);
    tcp_err(newpcb, tcp_server_err_callback);
    tcp_poll(newpcb, tcp_server_poll_callback, 4);

    printf("TCP Server: New client [%d] connected\r\n", slot);
    return ERR_OK;
}

static err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (p == NULL) {
        printf("TCP Server: Client disconnected\r\n");
        remove_client(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }

    char buffer[TCP_BUFFER_SIZE];
    u16_t copy_len = p->tot_len > (sizeof(buffer) - 1) ? (sizeof(buffer) - 1) : p->tot_len;
    pbuf_copy_partial(p, buffer, copy_len, 0);
    buffer[copy_len] = '\0';
    printf("TCP Server Recv: %s\r\n", buffer);

    tcp_write(pcb, buffer, copy_len, TCP_WRITE_FLAG_COPY);

    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    tcp_output(pcb);
    return ERR_OK;
}

static err_t tcp_server_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    return ERR_OK;
}

static void tcp_server_err_callback(void *arg, err_t err)
{
    struct tcp_pcb *pcb = (struct tcp_pcb *)arg;
    printf("TCP Server: Client error %d\r\n", err);
    remove_client(pcb);
}

static err_t tcp_server_poll_callback(void *arg, struct tcp_pcb *pcb)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    return ERR_OK;
}

void tcp_server_init(void)
{
    if (g_server_pcb != NULL) {
        printf("TCP Server: Already initialized\r\n");
        return;
    }

    g_server_pcb = tcp_new();
    if (g_server_pcb == NULL) {
        printf("TCP Server: Failed to create PCB\r\n");
        return;
    }

    ip_addr_t any_addr;
    ip_addr_set_any(false, &any_addr);

    err_t err = tcp_bind(g_server_pcb, &any_addr, TCP_SERVER_PORT);
    if (err != ERR_OK) {
        printf("TCP Server: tcp_bind failed, err=%d\r\n", err);
        tcp_close(g_server_pcb);
        g_server_pcb = NULL;
        return;
    }

    g_server_pcb = tcp_listen(g_server_pcb);
    if (g_server_pcb == NULL) {
        printf("TCP Server: tcp_listen failed\r\n");
        return;
    }

    tcp_accept(g_server_pcb, tcp_server_accept_callback);

    printf("TCP Server: Listening on port %d\r\n", TCP_SERVER_PORT);
}
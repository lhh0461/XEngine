#include "network.h"

void on_client_connection_recv(struct bufferevent *bev, void *ctx)
{
/*
    struct evbuffer * inbuf = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(inbuf);
    header_t *header = NULL;
    printf("on buffreadcb! len=%d\n", (int)len);
    if (len < sizeof(*header)) return;
    header = (header_t *)evbuffer_pullup(inbuf, -1);
    if (len < sizeof(*header) + header->paylen) return;
    evbuffer_drain(inbuf, sizeof(*header));
    int pid = header->pid;
    int paylen = header->paylen;
    printf("on buffreadcb! pid=%d,paylen=%d\n", pid, paylen);
    rpc_dispatch(0, pid, header + 1, paylen);
    evbuffer_drain(inbuf, sizeof(*header));
    */
}

void on_client_connection_write(struct bufferevent *bev, void *ctx)
{
    printf("on buffwritecb!\n");
}

void on_client_connection_event(struct bufferevent *bev, short what, void *ctx)
{
    printf("on buffevent!what=%d\n", what);
}

static void on_client_accept(evutil_socket_t connfd, short what, void * udata)
{
    printf("fd=%d connect!\n", connfd);
/*
    connection_new(connfd, on_client_connection_recv, on_client_connection_write, 
            on_client_connection_event, 1024 * 1024 * 1024, NULL);
    */
}

void open_client_service()
{
/*
    int listenfd = net_listen("127.0.0.1", "9999", 1024, 0);
    acceptor_t* acceptor = acceptor_new(listenfd, on_client_accept, NULL);
    */
}

void gated_init()
{

}

void gated_startup()
{
}

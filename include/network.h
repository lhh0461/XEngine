#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/bufferevent_compat.h>

#include "buffer.h"

typedef struct acceptor_s {
    int fd;
    struct event *ev;
    void *udata;
} acceptor_t;

typedef struct connection_s {
    int fd;
    struct bufferevent *bufev;
    void *udata;
} connection_t;

acceptor_t* acceptor_new(struct event_base *evbase, int fd, event_callback_fn acceptcb, void *udata);
void acceptor_free(acceptor_t *acceptor);

connection_t *connection_new(struct event_base *evbase, int fd, evbuffercb readcb, evbuffercb  writecb, 
        everrorcb errorcb, void *udata);
void connection_free(connection_t *conn);

int connection_send(connection_t *conn, const void *buf, size_t len);
int connection_send_packet(connection_t *conn, const void *header, size_t hdrlen, 
        const void *buf, size_t buflen);
int connection_send_buffer(connection_t *conn, buffer_t *buf);

int connection_try_flush(connection_t *conn);
int connection_force_flush(connection_t *conn);

#define READ_PACKET_CNT(header_type, bufev, arg, packet_handler, packet_cnt)    \
    do {                            \
        size_t bufsz = 0;               \
        size_t packet_sz = 0;               \
        size_t drain ; \
        header_type *header = NULL;         \
        int curcnt = 0; \
        while ((bufsz = evbuffer_get_length(bufferevent_get_input(bufev))) > 0) {       \
            if ((packet_cnt > 0 && curcnt >= packet_cnt) || bufsz < sizeof(*header))                      \
                break;                          \
            header = (header_type *)evbuffer_pullup(bufferevent_get_input(bufev),-1);   \
            packet_sz = sizeof(*header) + header->paylen;      \
            if (bufsz < packet_sz)                      \
                break;                              \
            (packet_handler)(arg, header, header + 1, header->paylen);    \
            drain = evbuffer_drain(bufferevent_get_input(bufev), packet_sz);            \
            assert(drain==0); \
            curcnt += 1; \
        }                                       \
    } while(0)

#define READ_PACKET(header_type, bufev, arg, packet_handler) \
    READ_PACKET_CNT(header_type, bufev, arg, packet_handler, -1)

#define READ_ONE_PACKET(header_type, bufev, arg, packet_handler) \
    READ_PACKET_CNT(header_type, bufev, arg, packet_handler, 1)


#endif //__NETWORK_H__


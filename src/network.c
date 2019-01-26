#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "network.h"
#include "common.h"

acceptor_t* acceptor_new(struct event_base * evbase, int fd, event_callback_fn acceptcb, void *udata)
{
    acceptor_t *acceptor = (acceptor_t *)calloc(1, sizeof(acceptor_t));
    if (!acceptor) return NULL;
    acceptor->fd = fd;
    acceptor->udata = udata;
    acceptor->ev = event_new(evbase, fd, EV_READ|EV_PERSIST, acceptcb, acceptor);
    event_add(acceptor->ev, NULL);
    return acceptor;
}

void acceptor_free(acceptor_t *acceptor)
{
    event_del(acceptor->ev);
    close(acceptor->fd);
    free(acceptor);
}

connection_t *connection_new(struct event_base *evbase, int fd, evbuffercb readcb, 
    evbuffercb  writecb, everrorcb errorcb, void *udata)
{
    connection_t *conn = (connection_t *)calloc(1, sizeof(connection_t));
    if (!conn) return NULL;
    conn->fd = fd;
    conn->bufev = bufferevent_socket_new(evbase,fd, BEV_OPT_CLOSE_ON_FREE); 
    if (!conn->bufev) {
        free(conn);
        return NULL;
    }
    bufferevent_setcb(conn->bufev, readcb, writecb, errorcb, conn); 
    bufferevent_enable(conn->bufev, EV_READ|EV_WRITE);
    conn->udata = udata;
    return conn;
}

void connection_free(connection_t *conn)
{
    bufferevent_free(conn->bufev);
    free(conn);
}

int connection_send(connection_t *conn, const void *buf, size_t len)
{
    int ret = bufferevent_write(conn->bufev, buf, len);
    if (ret != 0) {
        fprintf(stderr, "net send data error.bufsize=%lu,errno=%d\n",
                evbuffer_get_length(bufferevent_get_output(conn->bufev)), errno);
    }
    return ret;
}

int connection_send_packet(connection_t *conn, const void *header, size_t hdrlen, 
        const void *data, size_t pldlen)
{
    int ret;
    if ((ret = bufferevent_write(conn->bufev, header, hdrlen)) < 0) {
        ret = bufferevent_write(conn->bufev, data, pldlen);
        if (ret != 0) {
            fprintf(stderr, "net send packet data error.bufsize=%lu,errno=%d\n",
                    evbuffer_get_length(bufferevent_get_output(conn->bufev)), errno);
        }
    } else {
        fprintf(stderr, "net send packet header error.bufsize=%lu,errno=%d\n",
                evbuffer_get_length(bufferevent_get_output(conn->bufev)), errno);
    }
    return ret;
}

int connection_send_buffer(connection_t *conn, buffer_t *buf)
{
    if (buf->data_size > 0) {
        block_t *blk;

        for (blk = buf->head; blk != NULL; blk = blk->next) {
            unsigned len = BUFFER_BLOCK_DATA_LEN(blk);
            if (len > 0) {
                int ret = bufferevent_write(conn->bufev, blk->head, len);
                if (ret != 0) {
                    fprintf(stderr, "send to bufferevent error.bufsize=%lu,errno=%d\n",
                            evbuffer_get_length(bufferevent_get_output(conn->bufev)), errno);
                }
            }
        }
    }

    return buf->data_size;
}

int connection_try_flush(connection_t *conn)
{
    struct evbuffer *buffer = bufferevent_get_output(conn->bufev);
    size_t size = evbuffer_get_length(buffer);

    if (size > 0) {   
        evbuffer_unfreeze(buffer,1) ; 
        int w = evbuffer_write(buffer,  conn->fd); 
        return w < 0 ? -1 : w;
    }   
    return 0;
}

// force sending all data
int connection_force_flush(connection_t * conn)
{
    struct evbuffer * buffer = bufferevent_get_output(conn->bufev);
    size_t total = evbuffer_get_length(buffer);
    size_t left = total;
    int flag; // for restore
    set_block(conn->fd, 1, &flag);
    while (left > 0) {
        evbuffer_unfreeze(buffer,1);
        int w = evbuffer_write(buffer, conn->fd);
        if (w < 0) {
            fprintf(stderr, "%s:%s. total = %zu, left = %zu, errormsg = %s\n",
                    __FILE__, __func__, total, left, strerror(errno));
            return -1;
        }
        left -= w;
    }
    fcntl(conn->fd, F_SETFL, flag); // restore
    return total;
}


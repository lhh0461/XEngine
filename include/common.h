#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

int convert_addr(const char*ip, unsigned short port, struct sockaddr_in *addr);
int set_block(int fd, int block, int *oldflag);
int set_close_on_exec(int fd);
int net_connect(const char *ip, unsigned short port, int block);
int net_listen(const char *ip, unsigned short port, int backlog, int block);

int buffer_to_evbuffer(struct evbuffer *evbuf, buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif //__COMMON_H__

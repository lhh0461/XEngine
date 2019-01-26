#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <event2/buffer.h>

#include "common.h"

int convert_addr(const char*ip, unsigned short port, struct sockaddr_in *addr)
{
    struct in_addr inaddr;
    memset(&inaddr, 0, sizeof(inaddr));
    memset(addr, 0, sizeof(*addr));

    int ret = inet_aton(ip, &inaddr);
    if (1 == ret) {
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        addr->sin_addr = inaddr;
    }

    return ret;
}

int set_block(int fd, int block, int *oldflag)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (oldflag != NULL) {
        *oldflag = flag;
    }
    return fcntl(fd, F_SETFL, block == 0 ? flag | O_NONBLOCK : flag & (~O_NONBLOCK));
}

int set_close_on_exec(int fd)
{
    int flag = fcntl(fd, F_GETFD, 0);
    int result = fcntl(fd, F_SETFD, flag | FD_CLOEXEC);
    return result;
}

int net_connect(const char *ip, unsigned short port, int block)
{
    struct sockaddr_in addr;
    int fd;
    printf("connect to %s:%d\n", ip, port);
    if (1 == convert_addr(ip, port, &addr)) {
        fd = socket(PF_INET, SOCK_STREAM, 0);
        if (0 > fd) {
            perror("fs net socket");
            return -1;
        }
        if (!block) {
            if (0 > set_block(fd, 0, NULL)){
                perror("fs net fcntl noblock");
                close(fd);
                return -1;
            }
        }
        set_close_on_exec(fd);
        if (0 > connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
            if (!block && errno == EINPROGRESS) {
                return fd;
            }
            perror("fs net connect");
            close(fd);
            return -1;
        }

        return fd;
    } else {
        perror("convert addr fail");
    }

    return -1;
}

int net_listen(const char *ip, unsigned short port, int backlog, int block)
{
    struct sockaddr_in addr;
    int fd;
    fprintf(stderr, "listen on %s:%d\n", ip, port);
    if (1 == convert_addr(ip, port, &addr)) {
        fd = socket(PF_INET, SOCK_STREAM, 0);
        if (0 > fd) {
            perror("fs net socket");
            return -1;
        }
        int reuse = 1;
        if (0 > setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))) {
            perror("fs net setsockopt");
            close(fd);
            return -1;
        }
        if (0 > bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
            perror("fs net bind");
            close(fd);
            return -1;
        }
        if (0 > listen(fd, backlog)) {
            perror("fs net listen");
            close(fd);
            return -1;
        }
        if (!block) {
            if (0 > set_block(fd, 0, NULL)){
                perror("fs net fcntl noblock");
                close(fd);
                return -1;
            }
        }
        set_close_on_exec(fd);

        return fd;
    } else {
        perror("convert addr fail");
    }

    return -1;
}

int buffer_to_evbuffer(struct evbuffer *evbuf, buffer_t *buf)
{
    block_t *block = NULL;

    if (buf->data_size > 0) {
        for (block = buf->head; block != NULL; block = block->next) {
            unsigned len = BUFFER_BLOCK_DATA_LEN(block);
            if (len > 0) {
                evbuffer_add(evbuf, block->head, len);
            }
        }
    }

    return buf->data_size;
}


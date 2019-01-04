#include <string.h>
#include <event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proto_py/test.pb.h"

struct event_base *g_evbase;

void event_cb(evutil_socket_t fd, short what, void *arg)
{
    printf("on event!what=%d\n", what);
}

void buffreadcb(struct bufferevent *bev, void *ctx)
{
    printf("on buffreadcb!\n");
}

void buffwritecb(struct bufferevent *bev, void *ctx)
{
    printf("on buffwritecb!\n");
}

void buffeventcb(struct bufferevent *bev, short what, void *ctx)
{
    printf("on buffevent!what=%d\n", what);
}

const char *message = "hello world";
int main(int argc, const char **argv)
{
	g_evbase = event_base_new();
	if (!g_evbase) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

    struct event *ev = event_new(g_evbase, 0, EV_READ|EV_WRITE, event_cb, NULL);
    if (!ev) {
		fprintf(stderr, "Could not initialize event!\n");
		return 1;
    }
    struct sockaddr_in sin;

    bzero(&sin,sizeof(sin));  //初始化结构体
    sin.sin_family = AF_INET;  //设置地址家族
    sin.sin_port = htons(9999);  //设置端口
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");  //设置地址

    struct bufferevent *bev = bufferevent_socket_new(g_evbase, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, buffreadcb, buffwritecb, buffeventcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Could not initialize event!\n");
        return 1;
    }
    int pid = 1;
    evbuffer_add(bufferevent_get_output(bev), &pid, sizeof(pid));
    test::HelloRequest kReq;
    kReq.set_greeting("test hello !!!");
    size_t size = kReq.ByteSizeLong(); 
    void *buffer = malloc(size);
    kReq.SerializeToArray(buffer, size);
    evbuffer_add(bufferevent_get_output(bev), &size, sizeof(int));
    evbuffer_add(bufferevent_get_output(bev), buffer, size);
    free(buffer);
   
    printf("add data to buf success\n");

    event_add(ev, NULL);
    event_base_dispatch(g_evbase);

    return 0;
}

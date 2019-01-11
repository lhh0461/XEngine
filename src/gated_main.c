void buffreadcb(struct bufferevent *bev, void *ctx)
{
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
}

void buffwritecb(struct bufferevent *bev, void *ctx)
{
    printf("on buffwritecb!\n");
}

void buffeventcb(struct bufferevent *bev, short what, void *ctx)
{
    printf("on buffevent!what=%d\n", what);
}

static void on_client_accept(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
    printf("fd=%d connect!\n", fd);

    struct bufferevent *bev = bufferevent_socket_new(ev_base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, buffreadcb, buffwritecb, buffeventcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

void open_client_service()
{
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(9999);

    struct evconnlistener *listener;
    listener = evconnlistener_new_bind(ev_base, on_client_accept, NULL,
            LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
            (struct sockaddr*)&sin,
            sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
	}
}

#include <unistd.h>
#include <event2/event_struct.h>
#include <event2/event.h>

#include "log.h"
#include "timer.h"

struct event_base *ev_base = NULL;

static void
timeout_cb(void *arg)
{
    printf("on once timeout cb\n");
}

int main()
{
    struct event timeout;
    log_info("my server start!");

    ev_base = event_base_new();

    struct timeval tv,tv2;

    evutil_timerclear(&tv);
    tv.tv_sec = 3600;
    add_forever_timer(&tv, timeout_cb, (void *)NULL);

    event_base_dispatch(ev_base);
    return 0;
}

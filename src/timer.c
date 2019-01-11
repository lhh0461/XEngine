//定时器，引擎用
#include <stdlib.h>
#include <event2/event.h>

#include "timer.h"
#include "log.h"

extern struct event_base *ev_base;

static void timer_handler(int fd, short evnet, void *arg)
{
    evtimer_t* timer = (evtimer_t *)arg;

    (*timer->cb)(timer->arg);
    if (timer->type == T_FOREVER) {   
        return; 
    }

    evtimer_del(timer->evtimer);
    free(timer);
}

int add_timer(TIMER_TYPE type, struct timeval *tv, void (*cb)(void *), void * arg)
{
    evtimer_t *timer = (evtimer_t *)malloc(sizeof(evtimer_t));
    if (timer == NULL) {
        log_error("malloc timer fail");
        return -1;
    }

    timer->type = type;
    timer->evtimer = event_new(ev_base, -1, type == T_ONCE ? 0 : EV_PERSIST, timer_handler, timer);
    timer->cb = cb;
    timer->arg = arg;
    event_add(timer->evtimer, tv);
    
    return 0;
}

static void
timeout_cb(void *arg)
{
    printf("on timeout cb\n");
}

int init_timer()
{
    struct timeval tv;

    evutil_timerclear(&tv);
    tv.tv_sec = 10;
    add_once_timer(&tv, timeout_cb, (void *)NULL);
}

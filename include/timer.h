#ifndef __TIMER_H__
#define __TIMER_H__

typedef enum {
	T_ONCE    = 0x01,       /*一次定时*/
	T_FOREVER = 0x02,       /*永久定时*/
} TIMER_TYPE;

typedef struct evtimer_s {
    TIMER_TYPE type;        /*定时器类型*/
    struct event *evtimer;  /*定时事件处理器*/
    void (*cb)(void *);     /*定时器回调函数*/
    void *arg;              /*定时器回调参数*/
} evtimer_t;

int add_timer(TIMER_TYPE type, struct timeval *tv, void (*cb)(void *), void *arg);

#define add_once_timer(tv, cb, arg) add_timer(T_ONCE, (tv), (cb), (arg))
#define add_forever_timer(tv, cb, arg) add_timer(T_FOREVER, (tv), (cb), (arg))

#endif //__TIMER_H__

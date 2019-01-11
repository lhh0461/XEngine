#ifndef __TIMING_WHEEL_H__
#define __TIMING_WHEEL_H__

struct circq {
	struct circq *next;		/* next element */
	struct circq *prev;		/* previous element */
};

struct timeout {
	struct circq to_list;			/* timeout queue, don't move */
	void (*to_func)(struct timeout *,void *);/* fun to call */
	void *to_arg;				/* fun argument */
	int to_time;				/* ticks on event */
	int to_flags;				/* misc flags */
};

void timer_init(void)
void timeout_add(struct timeout *, void (*)(struct timeout *, void *), void *, int);
void timeout_del(struct timeout *);

void on_tick_update(void);

#endif	// __TIMING_WHEEL_H__

#include <stdlib.h>
#include <stdio.h>

#include "timeout.h"

#define BUCKETS 1024
#define WHEELSIZE 256
#define WHEELMASK 255
#define WHEELBITS 8

struct circq timeout_wheel[BUCKETS];	/* Queues of timeouts */
struct circq timeout_todo;		/* Worklist */

#define MASKWHEEL(wheel, time) (((time) >> ((wheel)*WHEELBITS)) & WHEELMASK)

#define BUCKET(rel, abs)						\
    (timeout_wheel[							\
     ((rel) <= (1 << (2*WHEELBITS)))			/*超时小于2^(8*2)*/	\
     ? ((rel) <= (1 << WHEELBITS))				\
     ? MASKWHEEL(0, (abs))				/*超时小于2^8*/	\
     : MASKWHEEL(1, (abs)) + WHEELSIZE	/*超时大于2^8但小于2^(8*2)*/		\
     : ((rel) <= (1 << (3*WHEELBITS)))				\
     ? MASKWHEEL(2, (abs)) + 2*WHEELSIZE		/*超时小于2^(8*3)*/	\
     : MASKWHEEL(3, (abs)) + 3*WHEELSIZE])   /*超时2^(8*3)*/

#define MOVEBUCKET(wheel, time)						\
    CIRCQ_APPEND(&timeout_todo,						\
            &timeout_wheel[MASKWHEEL((wheel), (time)) + (wheel)*WHEELSIZE])

/*
 * Circular queue definitions.
 */

#define CIRCQ_INIT(elem) do {                   \
    (elem)->next = (elem);                  \
    (elem)->prev = (elem);                  \
} while (0)

#define CIRCQ_INSERT(elem, list) do {           \
    (elem)->prev = (list)->prev;            \
    (elem)->next = (list);                  \
    (list)->prev->next = (elem);            \
    (list)->prev = (elem);                  \
} while (0)

#define CIRCQ_APPEND(fst, snd) do {             \
    if (!CIRCQ_EMPTY(snd)) {                \
        (fst)->prev->next = (snd)->next;\
        (snd)->next->prev = (fst)->prev;\
        (snd)->prev->next = (fst);      \
        (fst)->prev = (snd)->prev;      \
        CIRCQ_INIT(snd);                \
    }                                       \
} while (0)

#define CIRCQ_REMOVE(elem) do {                 \
    (elem)->next->prev = (elem)->prev;      \
    (elem)->prev->next = (elem)->next;      \
} while (0)

#define CIRCQ_FIRST(elem) ((elem)->next)

#define CIRCQ_EMPTY(elem) (CIRCQ_FIRST(elem) == (elem))

static int ticks = 0;		

void timer_init(void)
{
    int b;

    CIRCQ_INIT(&timeout_todo);
    for (b = 0; b < BUCKETS; b++)
        CIRCQ_INIT(&timeout_wheel[b]);
}

void timeout_add(struct timeout *new_, void (*fn)(struct timeout *, void *), void *arg int to_ticks)
{
    // 设置执行时间
    new_->to_func = fn;
    new_->to_arg = arg;
    new_->to_time = to_ticks + ticks;

    CIRCQ_INSERT(&new->to_list,
            &BUCKET((new_->to_time - ticks), new_->to_time));
}

void timeout_del(struct timeout *to)
{
    CIRCQ_REMOVE(&to->to_list);
}

static int _move_bucket()
{
    MOVEBUCKET(0, ticks);
    //到了第256秒的时候
    if (MASKWHEEL(0, ticks) == 0) {
        MOVEBUCKET(1, ticks);
        if (MASKWHEEL(1, ticks) == 0) {
            MOVEBUCKET(2, ticks);
            if (MASKWHEEL(2, ticks) == 0)
                MOVEBUCKET(3, ticks);
        }
    }

    return !CIRCQ_EMPTY(&timeout_todo);
}

static int _execute_timer()
{
    struct timeout *to;
    void (*fn)(struct timeout *, void *);
    void *arg;

    while (!CIRCQ_EMPTY(&timeout_todo)) {

        to = (struct timeout *)CIRCQ_FIRST(&timeout_todo);
        CIRCQ_REMOVE(&to->to_list);

        if (to->to_time - ticks > 0) {
            CIRCQ_INSERT(&to->to_list,
                    &BUCKET((to->to_time - ticks), to->to_time));
        } else {
            fn = to->to_func;
            arg = to->to_arg;
            fn(to, arg);
        }
    }
}

void on_tick_update(void)
{
    ticks++;
    if (_move_bucket()) {
        _execute_timer();
    }
}

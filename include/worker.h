#ifndef __WORKER_H__
#define __WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*worker_op_cb) (void *) OP_CB_FUNCTION_T;
typedef void *(*worker_init_cb) (void *) INIT_CB_FUNCTION_T;
typedef void *(*worker_quit_cb) (void *) QUIT_CB_FUNCTION_T;

typedef struct worker_s {
    int id;
    int fds[2];//fds[0] main thread fds[1] worker thread
    pthread_t pid;
    connection_t * main_conn; //主线程使用
    connection_t * worker_conn;//工作线程使用
    struct event_base *worker_evbase;
    OP_CB_FUNCTION_T worker_op_cb;
    INIT_CB_FUNCTION_T worker_init_cb;
    QUIT_CB_FUNCTION_T quit_cb;
} worker_t;

typedef struct workers_pool_s {
    int cnt;
    worker_t *workers;
} workers_pool_t;

#ifdef __cplusplus
}
#endif

#endif //__WORKER_H__


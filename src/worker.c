#include "worker.h"

void *worker_main(void *arg)
{
    worker_t *worker = (worker *)arg;
    if (worker->init_cb) {
        worker->init_cb(worker);
    }

    
    struct event_config *cfg = event_config_new();  
    if (cfg) {   
        event_config_set_flag(cfg, EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
        ev_base = event_base_new_with_config(cfg);
        event_config_free(cfg);  
    }
}

workers_pool_t * create_worker_pool(int num, main_thread_recv, op_cb, init_cb, quit_cb)
{
    worker_t *worker;
    workers_pool_t * pool = malloc(sizeof(workers_pool_t));
    if (pool == NULL) return NULL;

    for (int i = 0; i < num; i++) {
        worker = malloc(sizeof(worker_t));
        if (worker == NULL) {
            fprintf(stderr, "malloc fail");
            free(pool);
            return NULL;
        }
        if (socketpair(AF_UNIX, 0, SOCK_STREAM, worker->fds) < 0) {
            perror("socketpair");
            return 0;
        }
        set_block(worker->fds[0], 0, NULL);
        set_close_on_exec(worker->fds[0]);
        set_block(worker->fds[1], 0, NULL);
        set_close_on_exec(worker->fds[1]);
        
        int ret = pthread_create(&worker->pid, NULL, worker_main, worker);
        if (ret < 0) {
            fprintf(stderr, "create worker fail");
        }

        worker->main_base = connection_new(ev_base, worker->fds[0], main_thread_read_cb, NULL, everrorcb errorcb, void *udata)
    }
}


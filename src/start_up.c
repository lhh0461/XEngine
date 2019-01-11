#include <event2/event.h>
#include "script.h"

struct event_base *ev_base = NULL;

typedef void (*module_fun_t)(void);
module_fun_t module_init_fun = NULL;
module_fun_t module_startup_fun = NULL;

void init_libevent()
{
    struct event_config *cfg = event_config_new();  
    if (cfg) {   
        event_config_set_flag(cfg, EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
        ev_base = event_base_new_with_config(cfg);
        event_config_free(cfg);  
    }
}

void init()
{
    init_libevent();
    init_python_vm();
    create_rpc_table();

    if (module_init_fun != NULL) {
        module_init_fun();
    }
}

void startup()
{
    if (module_startup_fun != NULL) {
        module_start_up();
    }

    event_base_dispatch(ev_base);

    destroy_python_vm();
}

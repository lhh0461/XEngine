#include <unistd.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include "log.h"
#include "rpc.h"
#include "timer.h"
#include "script.h"
#include "start_up.h"

extern void gamed_init(void);
extern void gamed_startup(void);

extern void dbd_init(void);
extern void dbd_startup(void);

extern void gated_init(void);
extern void gated_startup(void);

void usage(void)
{
    printf("usage\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    char opt;
    log_info("server start!");

    module_init_fun = gamed_init;
    module_startup_fun = gamed_startup;

    while ((opt = getopt(argc, argv, "ldgh")) !=  - 1) {
        switch(opt) {
            case 'l':
                module_init_fun = gamed_init;
                module_startup_fun = gamed_startup;
                break;
            case 'd':
                module_init_fun = dbd_init;
                module_startup_fun = dbd_startup;
                break;
            case 'g':
                module_init_fun = gated_init;
                module_startup_fun = gated_startup;
                break;
            case '?':
            case 'h':
                usage();
                break;
            default:
                /* You won't actually get here. */
                printf("unknown opt=%d.\n", opt);
                exit(1);
        }
    }
    
    init();
    startup();
    return 0;
}

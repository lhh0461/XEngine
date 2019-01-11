#include <unistd.h>
#include <string.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include "log.h"
#include "rpc.h"
#include "timer.h"
#include "script.h"


void usage(void)
{
    printf("usage\n");
    exit(0);
}

int main(int argc, char *argc[])
{
    int opt;
    log_info("server start!");

    while ((opt = getopt(argc, argv, "f:AGTadbr:l:R:O:")) !=  - 1) {
        switch(opt) {
            case "l":
                module_init_fun = logicd_init;
                module_start_fun = logicd_startup;
                break;
            case "d":
                module_init_fun = dbd_init;
                module_start_fun = dbd_startup;
                break;
            case "g":
                module_init_fun = gated_init;
                module_start_fun = gated_startup;
                break;
            case "?":
            case "h":
                usage();
                break;
            default:
                /* You won't actually get here. */
                printf("unknown opt=%d.\n", opt);
                return;
        }
    }
    
    init();
    startup();

    return 0;
}

#include <unistd.h>
#include "log.h"

int main()
{
    LOG_INFO("server start!");
    LOG_ERROR("this is test log%d!", 1);

    while(1) {
        sleep(1);
    }
    return 0;
}

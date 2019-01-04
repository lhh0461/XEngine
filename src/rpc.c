#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.h"
#include "log.h"
#include "script.h"

static rpc_table_t g_rpc_table;

int call_c_function(rpc_function_t *functionp)
{

}

int rpc_dispatch(int uid, int pid, char *buf, int len)
{
    if (pid <= 0 || pid > g_rpc_table.size) return 1;
    rpc_function_t * functionp = g_rpc_table.table + pid;
    if (functionp->c_imp == 1) {
        return call_c_function(functionp);
    } else {
        return call_script_function(functionp->module, functionp->name, buf, len);
    }
}

int create_rpc_table()
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./proto/output/rpc.cfg", "r");
    if (fp == NULL) {
        printf("open rpc.cfg fail\n");
        return 1;
    }

    int function_num;
    getline(&line, &len, fp);
    sscanf(line, "function_table_num:%d", &function_num);
    g_rpc_table.table = (rpc_function_t *)calloc(sizeof(rpc_function_t), function_num);
    g_rpc_table.size = function_num;

    int id;
    char name[200];
    char module[200];
    int c_imp;
    int arg_count;
    rpc_function_t *functionp;
    while ((read = getline(&line, &len, fp)) != -1) {
       sscanf(line, "function_id:%d,cpp_imp:%d,arg_count:%d,module:%[^,]%*cfunction_name:%s\n", &id, &c_imp, &arg_count, module, name);
       rpc_function_t *functionp = g_rpc_table.table+id;
       if (functionp) {
           functionp->id = id;
           functionp->name = strdup(name);
           functionp->module = strdup(module);
           functionp->c_imp = c_imp;
       }
    }
    log_info("create rpc table success!");
}

#ifndef __RPC__
#define __RPC__

/*
#ifdef __cplusplus 
extern "C" { 
#endif
*/

typedef struct rpc_function_s
{
    int id;
    char *name;
    char *module;
    int c_imp;
} rpc_function_t;

typedef struct rpc_talbe_s
{
    rpc_function_t *table;
    int size;
} rpc_table_t;

int rpc_dispatch();
int create_rpc_table();

// 数据包头
typedef struct headr_s
{
    int pid;
    int paylen;
} header_t;

/*
#ifdef __cplusplus 
}
#endif
*/

#endif //__RPC__

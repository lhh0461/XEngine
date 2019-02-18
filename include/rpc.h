#ifndef __RPC__
#define __RPC__

#ifdef __cplusplus 
extern "C" { 
#endif

#include <msgpack.h>
#include <Python.h>

enum field_type_s {
    RPC_INT32 = 1,
    RPC_STRING = 2,
    RPC_STRUCT = 3,
    RPC_FLOAT = 4,
};

typedef struct rpc_field_s
{
    int type;
    char *name;
    int array;
    int struct_id;
} rpc_field_t;

typedef struct rpc_struct_s
{
    int field_cnt;
    rpc_field_t *field_list;
} rpc_struct_t;

typedef struct rpc_args_s
{
    int arg_cnt;
    rpc_field_t *arg_list;
} rpc_args_t;

typedef void (* rpc_c_function_t)(int id, PyObject *obj);

typedef struct rpc_function_s
{
    int id;
    char *name;
    char *module;
    int c_imp;
    int deamon;
    rpc_args_t args;
    rpc_c_function_t c_function;
} rpc_function_t;

typedef struct rpc_struct_talbe_s
{
    rpc_struct_t *table;
    int size;
} rpc_struct_table_t;

typedef struct rpc_function_talbe_s
{
    rpc_function_t *table;
    int size;
} rpc_function_table_t;

// 数据包头
typedef struct headr_s
{
    int pid;
    int paylen;
} header_t;

int create_rpc_table();
int rpc_dispatch(int id, char *buf, int len);
int rpc_send(int id, int pid, PyObject *obj);

#ifdef __cplusplus 
}
#endif

#endif //__RPC__


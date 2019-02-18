#ifndef __RPC_SCRIPT_H__
#define __RPC_SCRIPT_H__

#include <msgpack.h>

#include "rpc.h"
#include "unpack.h"

int pack(int pid, PyObject *obj, msgpack_sbuffer *sbuf);
PyObject * unpack(rpc_function_t *function, msgpack_unpacker_t *unpacker);
int rpc_script_dispatch(rpc_function_t *function, msgpack_unpacker_t *unpacker);

#endif //__RPC_SCRIPT_H__


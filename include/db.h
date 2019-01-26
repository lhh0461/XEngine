#ifndef __DB_H__
#define __DB_H__

#include <Python.h>
#include "marshal.h"
#include "protocol_dbd.h"

#define INDEXING 	"__indexing__"		// 集合索引 key

enum db_key_type_e {
    INT_TYPE = 1,
    STRING_TYPE,
};

enum db_save_method_e {
    SAVE_DATA = 0,
    SAVE_DIRTY = 1,
};

enum db_cmd_res_e {
    CMD_OK = 0,
    CMD_ERR = 1,
};

enum db_obj_state_e {
    STATE_OK = 1,
    STATE_ERROR = 2,
    STATE_NULL = 3,
    STATE_ONLINE = 4,
};

typedef struct db_key_s {
    int type;
    int len;
    void *value;
} db_key_t;

typedef struct db_object_s {
    db_key_t db_key;
    int hostid;
    PyObject *obj;
} db_object_t;

PyObject *db_object_recv(dg_msg_header_t *header, void *data, size_t len);
int load_dat_async(const char * docname, int sid);
PyObject *load_dat_sync(const char * docname, int sessionid);
int load_user_async(int uid, int sid);
void unmarshal_db_key(marshal_array_iter_t *iter, db_key_t *key);
void marshal_db_key(marshal_array_t *arr, db_key_t *key);

#endif //__DB_H__


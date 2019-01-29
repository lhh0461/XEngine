#ifndef __DB_H__
#define __DB_H__

#include <Python.h>
#include "marshal.h"
#include "protocol_dbd.h"

#define INDEXING 	"__indexing__"		// 集合索引 key

typedef enum db_key_type_e {
    INT_TYPE = 1,
    STRING_TYPE,
} db_key_type_t;

typedef enum db_save_method_e {
    SAVE_DATA = 0,
    SAVE_DIRTY = 1,
} db_save_method_t;

typedef enum db_cmd_res_e {
    CMD_OK = 0,
    CMD_ERR = 1,
} db_cmd_res_t;

typedef enum db_obj_state_e {
    STATE_OK = 1,
    STATE_ERROR = 2,
    STATE_NULL = 3,
    STATE_ONLINE = 4,
} db_obj_state_t;

typedef struct db_key_s {
    int type;
    int len;
    void *value;
} db_key_t;

typedef struct db_object_s {
    db_key_t db_key;
    int hostid;
    PyObject *obj;
} db_obj_t;

void unmarshal_db_key(marshal_array_iter_t *iter, db_key_t *key);
void marshal_db_key(marshal_array_t *arr, db_key_t *key);

PyObject *new_db_object(PyObject *docid);
PyObject *load_db_obj_sync(PyObject * docid, int sessionid, int *state);
int load_db_obj_async(PyObject *docid, int sid);
int save_db_obj(PyObject *docid, PyObject *obj, int save_method);
int unload_db_obj(PyObject *docid);
PyObject *recv_db_obj(dg_msg_header_t *header, void *data, size_t len);


#define LOG_MONGOC_ERROR(error)    \
    do {\
        fflush(NULL); \
        int code = error.code;     \
        char *msg = error.message; \
        printf("func:<<%s>> line:%d mongo op error!!!!! code:%d, message:%s.\n", __func__, __LINE__,code, msg); \
        fflush(NULL); \
    } while(0);

/*
#define DB_KEY_FORMAT(dbkey) \
        char _##dbkey_buf[1024] = {0}; \
        if ((dbkey)->type == INT_TYPE)  { \
            sprintf(_##dbkey_buf, "%d", *((int *)dbkey->value)) \
        } else {    \
            sprintf(_##dbkey_buf, "%s", dbkey->value) \
        }

#define DB_KEY_STR(dbkey) 
*/

#endif //__DB_H__


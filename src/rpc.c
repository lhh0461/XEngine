#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <msgpack.h>

#include "rpc.h"
#include "pydirty.h"
#include "unpack.h"
#include "log.h"
#include "script.h"

static rpc_function_table_t g_function_rpc_table;
static rpc_struct_table_t g_struct_rpc_table;

PyObject * unpack(rpc_function_t *function, msgpack_unpacker_t *unpacker);
int pack_field(rpc_field_t *field, PyObject *item, msgpack_packer *pck);
PyObject *unpack_field(rpc_field_t *field, msgpack_unpacker_t *unpacker);

rpc_struct_t * get_struct_by_id(int struct_id)
{
    if (struct_id <= 0 || struct_id > g_struct_rpc_table.size) return NULL;
    return g_struct_rpc_table.table + struct_id;
}

rpc_function_t * get_function_by_id(int function_id)
{
    if (function_id <= 0 || function_id > g_function_rpc_table.size) return NULL;
    return g_function_rpc_table.table + function_id;
}

int call_c_function(rpc_function_t *functionp)
{

}

int rpc_script_dispatch(rpc_function_t *function, msgpack_unpacker_t *unpacker)
{
    PyObject *obj = unpack(function, unpacker);
    if (obj != NULL) {
        return call_script_func(function->module, function->name, obj);
    }
    return -1;
}

//if return 0 success else fail
int rpc_dispatch(int uid, char *buf, int len)
{
    int pid = 0;
    msgpack_unpacker_t unpacker;
    construct_msgpack_unpacker(&unpacker, buf, len);
    if (unpacker.pack.data.type != MSGPACK_OBJECT_POSITIVE_INTEGER) {
        return 1;
    }
    pid = (int)unpacker.pack.data.via.u64;
    if (pid <= 0 || pid > g_function_rpc_table.size) return 1;

    rpc_function_t * function = g_function_rpc_table.table + pid;
    if (function->c_imp == 1) {
        //return rpc_c_dispatch(function, &unpacker);
    } else {
        return rpc_script_dispatch(function, &unpacker);
    }
    //TODO
error:
    destroy_msgpack_unpackert(&unpacker);
}

int check_field_type(int field_type, PyObject *item)
{
    switch(field_type) {
        case INT32:
            if (!PyLong_CheckExact(item)) return -1;
        case STRING:
            if (!PyUnicode_CheckExact(item)) return -1;
        case FLOAT:
            if (!PyFloat_CheckExact(item)) return -1;
        case STRUCT:
            if (!PyDict_CheckExact(item) && !PyDirtyDict_CheckExact(item)) return -1;
        default:
            fprintf(stderr, "unknown field type=%d", field_type);
            return -1;
    }
    return 0;
}

int pack_struct(rpc_struct_t *pstruct, PyObject *obj, msgpack_packer *pck)
{
    rpc_field_t *field;
    PyObject *item;
    assert(PyDict_CheckExact(obj) || PyDirtyDict_CheckExact(obj));

    for (int i = 0; i < pstruct->field_cnt; i++) {
        field = pstruct->field_list + i;
        item = PyObject_GetItem(obj, PyUnicode_FromString(field->name));
        if (field->array == 1) {
            if (!PyList_CheckExact(item)) {
                fprintf(stderr, "expected field type is list\n");
                return -1;
            }
            Py_ssize_t size = PyList_Size(item);
            msgpack_pack_array(pck, size); 
            for (int j = 0; j < pstruct->field_cnt; j++) {
                pack_field(field, obj, pck);
            }

        } else {
            pack_field(field, item, pck);
        }
    }
}

int pack_field(rpc_field_t *field, PyObject *item, msgpack_packer *pck)
{
    if (check_field_type(field->type, item) == -1) return -1;

    switch(field->type) {
        case INT32:
            msgpack_pack_uint32(pck, (uint32_t)PyLong_AsLong(item));
            break;
        case STRING: 
            {
                Py_ssize_t size; 
                const char *str = PyUnicode_AsUTF8AndSize(item, &size);
                msgpack_pack_str(pck, size); 
                msgpack_pack_str_body(pck, str, size); 
            }
            break;
        case FLOAT:
            msgpack_pack_double(pck, PyFloat_AS_DOUBLE(item));
            break;
        case STRUCT:
            {
                rpc_struct_t *pstruct = get_struct_by_id(field->struct_id);
                if (pstruct == NULL) {
                    fprintf(stderr, "field struct id invalid\n");
                    return -1;
                }
                pack_struct(pstruct, item, pck);
            }
            break;
        default:
            fprintf(stderr, "unknown field type=%d", field->type);
            return -1;
    }
}

int pack_args(rpc_args_t *args, PyObject *obj, msgpack_packer *pck)
{
    assert(PyTuple_CheckExact(obj));

    rpc_field_t *field;
    PyObject *item;

    for (int i = 0; i < args->arg_cnt; i++) {
        field = args->arg_list + i;
        item = PyTuple_GetItem(obj, i);
        if (field->array == 1) {
            if (!PyList_CheckExact(item)) {
                fprintf(stderr, "expected field type is list\n");
                return -1;
            }
            Py_ssize_t size = PyList_Size(item);
            msgpack_pack_array(pck, size); 
            for (int i = 0; i < args->arg_cnt; i++) {
                pack_field(field, item, pck);
            }

        } else {
            pack_field(field, item, pck);
        }
    }
}

//pack obj data to buf
int pack(int pid, PyObject *obj, msgpack_sbuffer *sbuf)
{
    if (!PyTuple_CheckExact(obj)) {
       return -1; 
    }

    rpc_function_t * function = get_function_by_id(pid);
    if (function == NULL) return -1;

    msgpack_sbuffer_init(sbuf);
    msgpack_packer pck;
    msgpack_packer_init(&pck, sbuf, msgpack_sbuffer_write);

    if (function->args.arg_cnt != PyTuple_Size(obj)) {
        return -1;
    }

    return pack_args(&function->args, obj, &pck);
}

PyObject *unpack_struct(rpc_struct_t *pstruce, msgpack_unpacker_t *unpacker)
{
    rpc_field_t *field;
    PyObject *key, *value, *item;
    msgpack_unpack_return ret;
    PyObject *dict = PyDict_New();

    for (int i = 0; i < pstruce->field_cnt; i++) {
        field = pstruce->field_list + i;
        if (field->array == 1) {
            ret = msgpck_unpacker_next_pack(unpacker);
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                if (GET_UNPACKER_PACK_TYPE(unpacker) != MSGPACK_OBJECT_ARRAY) {
                    fprintf(stderr, "expected field is array\n");
                    goto error;
                }
                value = PyTuple_New(unpacker->pack.data.via.array.size);
                for (unsigned int j = 0; j < unpacker->pack.data.via.array.size; j++) {
                    item = unpack_field(field, unpacker); 
                    if (value == NULL) {
                        goto error;
                    }
                    PyTuple_SetItem(value, j, item);
                }
            } else {
                fprintf(stderr, "unpack struct is error ret=%d\n", ret);
                goto error;
            }
        } else {
            value = unpack_field(field, unpacker); 
            if (value == NULL) {
                goto error;
            }
        }
        
        key = PyUnicode_FromString(field->name);
        PyDict_SetItem(dict, key, value);
    }
    return dict;
error:
    Py_DECREF(dict);
    return NULL; 
}

int check_unpack_field_type(int field_type, msgpack_unpacked *pack)
{
    switch (field_type) {
        case INT32:
            if (pack->data.type != MSGPACK_OBJECT_POSITIVE_INTEGER) return -1;
        case STRING:
            if (pack->data.type != MSGPACK_OBJECT_STR) return -1;
        case FLOAT:
            if (pack->data.type != MSGPACK_OBJECT_FLOAT32) return -1;
        default:
            fprintf(stderr, "unknown field type=%d\n", field_type);
            return -1;
    }
    return 0;
}


//return obj 正常
//return null 异常
PyObject *unpack_field(rpc_field_t *field, msgpack_unpacker_t *unpacker)
{
    msgpack_unpack_return ret;
    ret = msgpck_unpacker_next_pack(unpacker);
    if (ret == MSGPACK_UNPACK_SUCCESS) {
        if (check_unpack_field_type(field->type, &unpacker->pack) == -1) return NULL;

        switch(field->type) {
            case INT32:
                return PyLong_FromLong(unpacker->pack.data.via.u64);
            case STRING: 
                return PyUnicode_FromString(unpacker->pack.data.via.str.ptr);
            case FLOAT:
                return PyFloat_FromDouble(unpacker->pack.data.via.f64);
            case STRUCT:
                {
                    rpc_struct_t *pstruct = get_struct_by_id(field->struct_id);
                    if (pstruct == NULL) {
                        fprintf(stderr, "field struct id invalid\n");
                        return NULL;
                    }
                    return unpack_struct(pstruct, unpacker);
                }
            default:
                fprintf(stderr, "unknown field type=%d", field->type);
                return NULL;
        }
    } else {
        fprintf(stderr, "unpack is error ret=%d\n", ret);
        return NULL;
    }

    return NULL;
}

//把args塞入tuple
int unpack_args(rpc_args_t *args, PyObject *obj, msgpack_unpacker_t *unpacker)
{
    rpc_field_t *field;
    PyObject *value, *item;
    msgpack_unpack_return ret;

    assert(PyTuple_CheckExact(obj));

    for (int i = 0; i < args->arg_cnt; i++) {
        field = args->arg_list + i;
        if (field->array == 1) {
            ret = msgpck_unpacker_next_pack(unpacker);
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                if (GET_UNPACKER_PACK_TYPE(unpacker) != MSGPACK_OBJECT_ARRAY) {
                    fprintf(stderr, "expected field is array\n");
                    return -1;
                }
                value = PyTuple_New(unpacker->pack.data.via.array.size);
                for (unsigned j = 0; j < unpacker->pack.data.via.array.size; j++) {
                    item = unpack_field(field, unpacker); 
                    PyTuple_SetItem(value, j, item);
                }
            } else {
                fprintf(stderr, "unpack is error ret=%d\n", ret);
                return -1;
            }
        } else {
            value = unpack_field(field, unpacker);
        }
        PyTuple_SetItem(obj, i, value);
    }
    return 0;
}

//unpack buf data to obj
PyObject * unpack(rpc_function_t *function, msgpack_unpacker_t *unpacker)
{
    PyObject *obj;
    msgpack_unpack_return ret = msgpck_unpacker_next_pack(unpacker);
    if (ret == MSGPACK_UNPACK_SUCCESS) {
        if (GET_UNPACKER_PACK_TYPE(unpacker) != MSGPACK_OBJECT_ARRAY) {
            return NULL;
        }
        if ((unsigned)function->args.arg_cnt != unpacker->pack.data.via.array.size) {
            return NULL;
        }
        obj = PyTuple_New(unpacker->pack.data.via.array.size);
        if (unpack_args(&function->args, obj, unpacker)) {
            return NULL;
        }
        return obj;
    } else {
        fprintf(stderr, "unpacker unpack error.ret=%d\n", ret);
        return NULL;
    }
}

int create_rpc_table()
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./rpc.cfg", "r");
    if (fp == NULL) {
        printf("open rpc.cfg fail\n");
        return 1;
    }
    int struct_num;
    getline(&line, &len, fp);
    sscanf(line, "struct_table_num:%d", &struct_num);
    g_struct_rpc_table.table = (rpc_struct_t *)calloc(struct_num, sizeof(rpc_struct_t));
    g_struct_rpc_table.size = struct_num;

    int struct_id;
    char struct_name[200];
    char field_name[200];
    int field_count;
    rpc_struct_t *pstruct;
    rpc_field_t *field;
    for (int i = 0; i < struct_num; i++) {
        getline(&line, &len, fp);
        sscanf(line, "struct_id:%d,field_num:%d,struct_name=%s", &struct_id, &field_count, struct_name);
        pstruct = g_struct_rpc_table.table + i;
        pstruct->field_cnt = field_count;
        pstruct->field_list = calloc(field_count, sizeof(rpc_field_t));
        for (int j = 0; j < field_count; j++) {
            field = pstruct->field_list + j;
            getline(&line, &len, fp);
            sscanf(line, "field_type:%d,struct_id:%d,array:%d,field_name=%s", &field->type, &field->struct_id, &field->array, field_name);
            field->name = strdup(field_name);
        }
    }

    int function_num;
    getline(&line, &len, fp);
    sscanf(line, "function_table_num:%d", &function_num);
    g_function_rpc_table.table = (rpc_function_t *)calloc(function_num, sizeof(rpc_function_t));
    g_function_rpc_table.size = function_num;

    int id;
    char name[200];
    char module[200];
    int c_imp;
    int arg_count;
    rpc_function_t *functionp;
    while ((read = getline(&line, &len, fp)) != -1) {
       sscanf(line, "function_id:%d,cpp_imp:%d,arg_count:%d,module:%[^,]%*cfunction_name:%s\n", &id, &c_imp, &arg_count, module, name);
       rpc_function_t *functionp = g_function_rpc_table.table+id;
       if (functionp) {
           functionp->id = id;
           functionp->name = strdup(name);
           functionp->module = strdup(module);
           functionp->c_imp = c_imp;
       }
    }
    log_info("create rpc table success!");
}

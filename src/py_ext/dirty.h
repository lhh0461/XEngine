#ifndef __DIRTY_H__
#define __DIRTY_H__

#include <Python.h>
#include "list.h"

enum dirty_op_e
{
    DIRTY_SET_OP = 1,
    DIRTY_DEL_OP = 2,
};

typedef struct dirty_node_s
{
    struct dirty_mng_s *mng;
    PyDictObject *dirty_key_dict;
} dirty_node_t;

typedef struct dirty_root_s
{
    list_t *dirty_node_list;
} dirty_root_t;

typedef struct dirty_mng_s
{
    PyObject *parent;
    PyObject *root;
    // value
    PyObject *self;
    // key
    PyObject *key;
    dirty_node_t *dirty_node;
    dirty_root_t dirty_root[0];
} dirty_mng_t;


void set_dirty_data(PyObject *self, PyObject *key, enum dirty_op_e op);
void clean_dirty(PyObject *self);

#endif //__DIRTY_H__

#ifndef __DIRTY_H__
#define __DIRTY_H__

#include <Python.h>
#include "pydirty.h"
#include "marshal.h"
#include "queue.h"

enum dirty_op_e
{
    DIRTY_ADD_OP = 1,
    DIRTY_SET_OP = 2,
    DIRTY_DEL_OP = 3,
};

typedef struct dirty_node_s
{
    TAILQ_ENTRY(dirty_node_s) entry;
    unsigned key_cnt;
    struct dirty_mng_s *mng;
    PyDictObject *dirty_key_dict;
} dirty_node_t;

typedef struct dirty_root_s
{
    TAILQ_HEAD(dirty_node_head_s, dirty_node_s) dirty_node_list;
    unsigned node_cnt;
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

#define get_manage(sv) ( PyDirtyDict_CheckExact(sv) ? ((PyDirtyDictObject *)sv)->dirty_mng : (PyDirtyList_CheckExact(sv) ? ((    PyDirtyListObject *)sv)->dirty_mng : NULL ) )

#define IS_DIRTY_ROOT(mng) ((mng)->self == (mng)->root && (mng)->root != NULL)


void set_dirty_dict(PyDirtyDictObject *self, PyObject *key, enum dirty_op_e op);
void set_dirty_list(PyDirtyListObject *self, PyObject *key, enum dirty_op_e op);

void clear_dirty(PyObject *self);
void dirty_mem_pool_setup(void);

void begin_dirty_manage_dict(PyDirtyDictObject *svmap, PyObject *parent, PyObject *skey);
void begin_dirty_manage_list(PyDirtyListObject *svarr, PyObject *parent, PyObject *skey);

void handle_old_dirty_subdocs(PyDirtyDictObject *dict);
void handle_new_dirty_subdocs(PyDirtyDictObject *dict);

void free_dirty_arr_recurse(PyDirtyListObject *arr);
PyObject *get_dirty_info(PyObject *v);
PyObject *dump_dirty_info(PyObject *v);

int marshal_dirty_dict(PyDirtyDictObject *mp, marshal_array_t *arr);

#endif //__DIRTY_H__

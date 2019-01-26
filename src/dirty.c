#include <Python.h>
#include <glib.h>
#include <structmember.h>

#include "dirty.h"
#include "pydirty.h"
#include "queue.h"
#include "log.h"
#include "mem_pool.h"

static mem_pool_t *pool_node;
static mem_pool_t *pool_manage;
static mem_pool_t *pool_manage_root;

#define DIRTY_DUMP_DEBUG 1

static const char *PyObject_Str_fail = "PyObject_Str_fail";
const char *save_variable(PyObject *obj)
{
    PyObject *str = PyObject_Str(obj);
    if (str != NULL) {
        return PyUnicode_AS_DATA(str);
    } else {
        return PyObject_Str_fail;
    }
}

void dirty_mem_pool_setup(void)
{
    pool_node = create_memory_pool(sizeof(dirty_node_t), 1024);
    pool_manage = create_memory_pool(sizeof(dirty_mng_t), 1024);
    pool_manage_root = create_memory_pool(sizeof(dirty_mng_t) + sizeof(dirty_root_t), 128);
}

void dirty_mem_pool_clear(void)
{

}

static void free_dirty_dict(PyDirtyDictObject *map);
static void free_dirty_arr(PyDirtyListObject *arr);

static void free_dirty_dict_recurse(PyDirtyDictObject *map);

static void clear_dirty_dict(PyDirtyDictObject *map);
static void clear_dirty_arr(PyDirtyListObject *arr);

static void clear_dirty_arr_recurse(PyDirtyListObject *arr);
static void clear_dirty_map_recurse(PyDirtyDictObject *map);

inline static void overwrite_dirty_key(dirty_node_t *node, PyObject *key, unsigned char newop)
{
    unsigned char oldop = (unsigned char)PyLong_AS_LONG(PyDict_GetItem((PyObject *)node->dirty_key_dict, key));
    if (oldop == newop) return;

    int op = 0;
    if (oldop == DIRTY_DEL_OP) {
        if (newop == DIRTY_ADD_OP) {
            op = DIRTY_SET_OP;
        } else {
            op = newop;
        }
    } else if (oldop == DIRTY_ADD_OP) {
        if (newop == DIRTY_DEL_OP) {
            PyDict_DelItem((PyObject *)node->dirty_key_dict, key);
            return;
        } else if (newop == DIRTY_SET_OP) {
            // 先DIRTY_ADD再DIRTY_SET，应该保持DIRTY_ADD状态
            return;
        } else {
            log_error("please check code, add dirty data twice!");
            return;
        }
    } else {
        if (newop == DIRTY_DEL_OP) {
            op = newop;
        } else if (newop == DIRTY_ADD_OP) {
            log_error("please check code, add dirty data when it was set!");
            return;
        }
    }

    printf("overwrite dirty key=%p,op=%d\n", key, op);
    PyDict_SetItem((PyObject *)node->dirty_key_dict, key, PyLong_FromLong(op));
}

static PyObject *insert_dict_key(dirty_node_t* dirty_node, PyObject* key, unsigned char op)
{
    printf("insert dirty key=%p,op=%d\n", key, op);
    PyDict_SetItem((PyObject *)dirty_node->dirty_key_dict, key, PyLong_FromLong(op));
    dirty_node->key_cnt++;
    return key;
} 

inline static void dirty_root_init(dirty_root_t *dirty_root)
{
    TAILQ_INIT(&dirty_root->dirty_node_list);
    dirty_root->node_cnt = 0;
}

inline static void dirty_root_add(dirty_root_t *dirty_root, dirty_node_t *dirty_node)
{
    printf("add dirty node. root=%p,node=%p\n", dirty_root, dirty_node);
    TAILQ_INSERT_TAIL(&dirty_root->dirty_node_list, dirty_node, entry);
    dirty_root->node_cnt++;
}

inline static void dirty_root_remove(dirty_root_t *dirty_root, dirty_node_t *dirty_node)
{
    printf("remove dirty node. root=%p,node=%p\n", dirty_root, dirty_node);
    TAILQ_REMOVE(&dirty_root->dirty_node_list, dirty_node, entry);
    dirty_root->node_cnt--;
}

static dirty_node_t *new_dirty_node(dirty_mng_t *mng)
{
    dirty_node_t *dirty_node = (dirty_node_t *)malloc_node(pool_node);
    printf("new dirty node:%p\n", dirty_node);

    dirty_node->key_cnt = 0;
    dirty_node->dirty_key_dict = (PyDictObject *)PyDict_New();

    dirty_mng_t *root_mng = get_manage(mng->root);
    dirty_root_add(root_mng->dirty_root, dirty_node);

    dirty_node->mng = mng;
    mng->dirty_node = dirty_node;

    return dirty_node;
}

static void free_dirty_node(dirty_mng_t *mng)
{
    printf("new dirty mng:%p\n", mng);
    dirty_root_t *dirty_root  = get_manage(mng->root)->dirty_root;
    dirty_root_remove(dirty_root, mng->dirty_node);

    mng->dirty_node->mng = NULL;
    free_node(pool_node, mng->dirty_node);
    Py_DECREF(mng->dirty_node->dirty_key_dict);
    mng->dirty_node = NULL;
}

//todo: Add or Set 入dirty map的value，要检测其是否已经在别的dirty map中，然后再挂入dirty map.
inline static void attach_node(PyObject *value, PyObject *parent, PyObject *key)
{
    printf("attach node value:%p, parent:%p, key:%p\n", value, parent, key);
    if (PyDirtyDict_CheckExact(value)) {
        begin_dirty_manage_dict((PyDirtyDictObject *)value, parent, key);
    } else if (PyDirtyList_CheckExact(value)) {
        begin_dirty_manage_list((PyDirtyListObject *)value, parent, key);
    }
}

//延迟处理，在清理脏数据的key的时候才把新增进来的mapping启动脏数据管理
static void clear_dirty_node(dirty_mng_t *mng)
{
    dirty_node_t *dirty_node = mng->dirty_node;
    PyObject *node = mng->self;

    assert(dirty_node);

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    if (PyDirtyDict_CheckExact(node)) {
        while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &value)) {
            switch ((int)PyLong_AS_LONG(value)) {
                case DIRTY_SET_OP: {
                    attach_node(PyDict_GetItem(node, key), node, key);
                    break;
                }
            }
        }
    } else if(PyDirtyList_CheckExact(node)) {
        while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &value)) {
            switch ((int)PyLong_AS_LONG(value)) {
                case DIRTY_SET_OP: {
                    attach_node(PyList_GET_ITEM(node, PyLong_AS_LONG(key)), node, key);
                    break;
                }
            }
        }
    } else {
        assert(0);
    }

    free_dirty_node(mng);
}

inline static void dirty_manage_init(dirty_mng_t *mng, PyObject *self, PyObject *root, PyObject *parent, PyObject *key)
{
    mng->root = root;
    mng->parent = parent;
    mng->self = self;
    mng->key = key;
    mng->dirty_node = NULL;
}

static void free_dirty_manage(dirty_mng_t *mng)
{
    int is_root = IS_DIRTY_ROOT(mng);
    if (mng->dirty_node) {
        free_dirty_node(mng);
    }
    mng->root = NULL;
    mng->parent = NULL;
    mng->self = NULL;

    if (is_root) {
        free_node(pool_manage_root, mng);
    } else {
        free_node(pool_manage, mng);
    }
}

static dirty_mng_t *new_dirty_manage(PyObject *self, PyObject *parent, PyObject *key)
{
    dirty_mng_t *mng;

    if (parent == NULL) { //the root 
        mng = (dirty_mng_t *)malloc_node(pool_manage_root);
        dirty_manage_init(mng, self, self, NULL, NULL);
        dirty_root_init(mng->dirty_root);
    } else {
        mng = (dirty_mng_t *)malloc_node(pool_manage);
        PyObject *root = get_manage(parent)->root;
        dirty_manage_init(mng, self, root, parent, key);
    }

    return mng;
}

void clear_dirty_manage(dirty_mng_t *mng)
{
    if (mng != NULL) {
        if (mng->dirty_node) {
            clear_dirty_node(mng);
        }
        //need not to handle dirty_root
        //dirty_root is affected by dirty_node of manage
    }
}

static int marshal_dirty_path(dirty_manage_t *mng, marshal_array_t *arr)
{
    void *dbi_arr;
    int c = 0;
    dbi_arr = marshal_array_alloc(arr, sizeof(marshal_arr_t));

    for ( ; mng->parent; mng = get_manage(mng->parent), c++) {
        if (PyDirtyDict_CheckExact(mng->parent)) {
            PyObject *key = mng->self_key.map_key;

            if (PyInt_CheckExact(key)) {
                if (marshal(key, arr)) return -1;
            } else if (PyFloat_CheckExact(key)) {
                //marshal_array_push_real(arr, key->u.real);
                //marshal_array_push_double(arr, key->u.real);
                if (marshal(key, arr)) return -1;
            } else if (PyString_CheckExact(key)) {
                //marshal_array_push_lstring(arr, key->u.string, SVALUE_STRLEN(key));
                if (marshal(key, arr)) return -1;
            } else {
                fprintf(stderr, "[marshal dirty] %s:%d invalid key type:%s\n", __FILE__, __LINE__, Py_TYPE(key)->tp_name);
                return -1;
            }
        } else if (PyDirtyList_CheckExact(mng->parent)) {
            marshal_array_push_int(arr, mng->self_key.arr_index);
        } else {
            fprintf(stderr, "[marshal dirty] %s:%d invalid type:%s\n", __FILE__, __LINE__, Py_TYPE(mng->parent)->tp_name);
            return -1;
        }
    }

    marshal_array_set_array(arr, dbi_arr, c);
    return 0;
}

static int marshal_dirty_list_node(PyDirtyListObject *node, marshal_array_t *arr)
{
    dirty_manage_t *mng = node->dirty_mng;
    dirty_node_t *dirty_node = mng->dirty_node;
    PyObject *value;

    marshal_dirty_path(mng, arr);
    marshal_array_push_array(arr, dirty_node->key_cnt);
    //printf("map dirty key size:%d\n", dirty_node->key_cnt);

    PyObject *key, *op;
    Py_ssize_t pos = 0;
    while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &op)) {
        int ikey = (int)PyInt_AS_LONG(key);
        switch ((int)PyInt_AS_LONG(op)) {
            case DIRTY_SET_OP:
            case DIRTY_ADD_OP:
                value = PyList_GET_ITEM(node, ikey);
                //printf("add or set dirty map k:%s,v:%s\n", save_variable(key), save_variable(value));
                marshal_array_push_int(arr, ikey);
                if (marshal(value, arr)) return -1;
                break;
            case DIRTY_DEL_OP:
                marshal_array_push_int(arr, ikey);
                marshal_array_push_nil(arr);
                //printf("del dirty map k:%s\n", save_variable(&dk->key.del));
                break;
        }
    }
    return 0;
}

static int marshal_dirty_dict_node(PyDirtyDictObject *node, marshal_array_t *arr)
{
    dirty_manage_t *mng = node->dirty_mng;
    dirty_node_t *dirty_node = mng->dirty_node;
    PyObject *key, *value;

    marshal_dirty_path(mng, arr);
    marshal_array_push_mapping(arr, dirty_node->key_cnt);
    //printf("map dirty key size:%d\n", dirty_node->key_cnt);

    PyObject *op;
    Py_ssize_t pos = 0;
    while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &op)) {
        switch ((int)PyInt_AS_LONG(op)) {
            case DIRTY_SET:
            case DIRTY_ADD:
                value = PyDict_GetItem((PyObject *)node, key);
                //printf("add or set dirty map k:%s,v:%s\n", save_variable(key), save_variable(value));
                if (marshal(key, arr)) return -1;
                if (marshal(value, arr)) return -1;
                break;
            case DIRTY_DEL:
                if (marshal(key, arr)) return -1;
                marshal_array_push_nil(arr);
                //printf("del dirty map k:%s\n", save_variable(&dk->key.del));
                break;
        }
    }

    return 0;
}

int marshal_dirty_dict(PyDirtyDictObject *mp, marshal_array_t *arr)
{
    dirty_manage_t *root_mng = get_manage(mp);
    dirty_root_t *dirty_root = root_mng->dirty_root;
    dirty_node_t *dirty_node;
    PyObject *node;

    //printf("dirty map entry:%d\n", dirty_root->node_cnt);
    marshal_array_push_mapping(arr, dirty_root->node_cnt);

    TAILQ_FOREACH(dirty_node, &dirty_root->dirty_node_list, entry) {
        node = dirty_node->mng->self;
        if (PyDirtyDict_CheckExact(node)) {
            if (marshal_dirty_dict_node((PyDirtyDictObject *)node, arr)) return -1; 
        } else if (PyDirtyList_CheckExact((PyDirtyListObject *)node)) {
            if (marshal_dirty_list_node((PyDirtyListObject *)node, arr)) return -1; 
        } else {
            fprintf(stderr, "[marshal dirty] %s:%d invalid dirty_node type %s\n", __FILE__, __LINE__, Py_TYPE(node)->tp_name);
            return -1; 
        }   
    }   

    return 0;
}
    

inline static void detach_node(int op, PyObject *value)
{
    //todo check the added content if dirty
    //op == DIRTY_ADD has to delay, we do not know what will be added.
    //此刻不知道ADD或者SET进来的数据是什么，要延迟做检查处理。
#ifdef DIRTY_MAP_CHECK
    assert_detach(op, value);
#endif

    printf("detach node\n");
    if (op == DIRTY_ADD_OP) return;
    //printf("detach node key value=%x\n", value);

    if (PyDirtyDict_CheckExact(value)) {
        printf("detach node key7\n");
        free_dirty_dict_recurse((PyDirtyDictObject *)value);
    } else if (PyDirtyList_CheckExact(value)) {
        printf("detach node key8\n");
        free_dirty_arr_recurse((PyDirtyListObject *)value);
    }
    printf("detach node key9\n");
}

inline static void accept_dirty_key(PyObject *ob, PyObject *key, unsigned char op)
{
    PyObject *value = NULL;
    dirty_mng_t *mng = NULL;

    if (PyDirtyDict_CheckExact(ob)) {
        value = PyDict_GetItem(ob, key);
        mng = ((PyDirtyDictObject *)ob)->dirty_mng;

    } else if (PyDirtyList_CheckExact(ob)) {
        int idx = PyLong_AsLong(key);
        value = PyList_GET_ITEM(ob, idx);
        mng = ((PyDirtyListObject *)ob)->dirty_mng;

    } else {
        assert(0);
    }

    if (mng->dirty_node == NULL) {
        new_dirty_node(mng); 
    }
    printf("accept dirty key\n");
    
    PyObject *parent = mng->parent;
    PyObject *skey = mng->key;
    while (parent != NULL) {
        dirty_mng_t *parent_mng = get_manage(parent);
        if (parent_mng->dirty_node) {
            //上层已经设了脏，本层数据修改不用再设脏
            if (PyDict_Contains((PyObject *)parent_mng->dirty_node->dirty_key_dict, skey)) {
                return;
            }
        }
        parent = parent_mng->parent;
        skey = parent_mng->key;
    }

    if (PyDict_Contains((PyObject *)mng->dirty_node->dirty_key_dict, key)) {
        detach_node(op, value);
        overwrite_dirty_key(mng->dirty_node, key, op);
    } else {
        insert_dict_key(mng->dirty_node, key, op);
        detach_node(op, value);
    }
}

void set_dirty_dict(PyDirtyDictObject *dict, PyObject *key, enum dirty_op_e op)
{
    if (dict->dirty_mng) {
        accept_dirty_key((PyObject *)dict, key, op);
    }
}

void set_dirty_list(PyDirtyListObject *list, PyObject *key, enum dirty_op_e op)
{
    if (list->dirty_mng) {
        accept_dirty_key((PyObject *)list, key, op);
    }
}

static void clear_dirty_dict(PyDirtyDictObject *dict)
{
    if (dict->dirty_mng) {
        clear_dirty_manage(dict->dirty_mng);
    }
}

void free_dirty_dict(PyDirtyDictObject *dict)
{
    if (dict->dirty_mng) {
        free_dirty_manage(dict->dirty_mng);
        dict->dirty_mng = NULL;
    }
}

static void clear_dirty_arr(PyDirtyListObject *arr)
{
    if (arr->dirty_mng) {
        clear_dirty_manage(arr->dirty_mng);
    }
}

void free_dirty_arr(PyDirtyListObject *arr)
{
    if (arr->dirty_mng) {
        free_dirty_manage(arr->dirty_mng);
        arr->dirty_mng = NULL;
    }
}

static void clear_dirty_map_recurse(PyDirtyDictObject *map)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    clear_dirty_dict(map);

    while (PyDict_Next((PyObject *)map, &pos, &key, &value)) {
        if (PyDirtyDict_CheckExact(value)) {
            clear_dirty_map_recurse((PyDirtyDictObject *)value);
        } else if (PyDirtyList_CheckExact(value)) {
            clear_dirty_arr_recurse((PyDirtyListObject *)value);
        }
    }
}

static void free_dirty_dict_recurse(PyDirtyDictObject *map)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next((PyObject *)map, &pos, &key, &value)) {
        if (PyDirtyDict_CheckExact(value)) {
            free_dirty_dict_recurse((PyDirtyDictObject *)value);
        } else if (PyDirtyList_CheckExact(value)) {
            free_dirty_arr_recurse((PyDirtyListObject *)value);
        }
    }

    //root要最后清除,但这样就不能尾递归了
    free_dirty_dict(map);
}

void begin_dirty_manage_dict(PyDirtyDictObject *svmap, PyObject *parent, PyObject *skey)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    if (!PyDirtyDict_CheckExact(svmap)) {
        return;
    }

    if (svmap->dirty_mng) {
        return;
    }

    svmap->dirty_mng = new_dirty_manage((PyObject *)svmap, parent, skey);
    printf("begin_dirty_manage_dict\n");

    while (PyDict_Next((PyObject *)svmap, &pos, &key, &value)) {
        if (PyDirtyDict_CheckExact(value)) {
            begin_dirty_manage_dict((PyDirtyDictObject *)value, (PyObject *)svmap, key);
        } else if (PyDirtyList_CheckExact(value)) {
            begin_dirty_manage_list((PyDirtyListObject *)value, (PyObject *)svmap, key);
        }
    }
    PyObject_Print((PyObject *)svmap, stdout, 0);
}

void begin_dirty_manage_list(PyDirtyListObject *svarr, PyObject *parent, PyObject *skey)
{
    size_t i, size;
    PyObject *item;

    if (!PyDirtyList_CheckExact(svarr)) {
        return;
    }

    if (svarr->dirty_mng) {
        return;
    }

    printf("begin_dirty_manage_list\n");

    svarr->dirty_mng = new_dirty_manage((PyObject *)svarr, parent, skey);
    size = PyList_GET_SIZE(svarr);
    for (i = 0; i < size; i++) {
        item = PyList_GET_ITEM(svarr, i);
        if (PyDirtyDict_CheckExact(item)) {
            begin_dirty_manage_dict((PyDirtyDictObject *)item, (PyObject *)svarr, PyLong_FromLong(i));
        } else if (PyDirtyList_CheckExact(item)) {
            begin_dirty_manage_list((PyDirtyListObject *)item, (PyObject *)svarr, PyLong_FromLong(i));
        }
    }
}

static void clear_dirty_arr_recurse(PyDirtyListObject *arr)
{
    size_t i, size;
    PyObject *item;

    clear_dirty_arr(arr);
    size = PyList_GET_SIZE(arr);

    for (i = 0; i < size; i++) {
        item = PyList_GET_ITEM(arr, i);
        if (PyDirtyDict_CheckExact(item)) {
            clear_dirty_map_recurse((PyDirtyDictObject *)item);
        } else if (PyDirtyList_CheckExact(item)) {
            clear_dirty_arr_recurse((PyDirtyListObject *)item);
        }
    }
}

void free_dirty_arr_recurse(PyDirtyListObject *arr)
{
    size_t i, size;
    PyObject *item;
    size = PyList_GET_SIZE(arr);

    for (i = 0; i < size; i++) {
        item = PyList_GET_ITEM(arr, i);
        if (PyDirtyDict_CheckExact(item)) {
            free_dirty_dict_recurse((PyDirtyDictObject *)item);
        } else if (PyDirtyList_CheckExact(item)) {
            free_dirty_arr_recurse((PyDirtyListObject *)item);
        }
    }

    //root要最后清除,但这样就不能尾递归了
    free_dirty_arr(arr);
}

//从根开始清理脏数据
void clear_dirty(PyObject *value)
{
    dirty_mng_t *mng = get_manage(value);
    if (mng != NULL) {
        assert(IS_DIRTY_ROOT(mng));
        PyObject *node;
        dirty_node_t *dirty_node, *next;
        dirty_root_t *dirty_root = mng->dirty_root;

        TAILQ_FOREACH_SAFE(dirty_node, &dirty_root->dirty_node_list, entry, next) {
            node = dirty_node->mng->self;
            if (PyDirtyDict_CheckExact(node)) {
                clear_dirty_dict((PyDirtyDictObject *)node);
            } else if (PyDirtyList_CheckExact(node)) {
                clear_dirty_arr((PyDirtyListObject *)node);
            }
        }
    }
}

static PyObject *get_node_from_dirty_manager(dirty_mng_t *mng, PyDictObject *root)
{
    int c = 0, i;
    dirty_mng_t *src = mng;
    PyObject *key = NULL, *value = NULL, *node;

    if (!mng->parent) {
        return (PyObject *)root;
    }

    for (c = 0, mng = src ; mng->parent; mng = get_manage(mng->parent), c++) {

    }

    dirty_mng_t **mngs = (dirty_mng_t **)calloc(c, sizeof(dirty_mng_t *));

    for (c = 0, mng = src ; mng->parent; mng = get_manage(mng->parent), c++) {
        *(mngs + c) = mng;
    }

    for (i = c - 1, node = (PyObject *)root; i >= 0; --i) {
        mng = *(mngs + i);
        int need_decref_key = 0;
        if (PyDirtyDict_CheckExact(mng->parent)) {
            key = mng->key;
            need_decref_key = 1;
        } else if (PyDirtyList_CheckExact(mng->parent)) {
            key = mng->key;
            need_decref_key = 1;
        } else {
            fprintf(stderr, "%s:%d get_node_from_dirty_manager error\n", __FILE__, __LINE__);
            assert(0);
        }

        printf("get_node_from_dirty_manager node=%p,key=%p\n", node, key);
        if (!PyDict_Contains(node, key)) {
            value = PyDict_New();
            PyDict_SetItem(node, key, value);
            if (need_decref_key) {
                Py_DECREF(key);
            }
            Py_DECREF(value);
        } else {
            value = PyDict_GetItem(node, key);
        }
        node = value;
    }

    free((void **)mngs);
    return value;
}

static void get_map_dirty_info(PyDirtyDictObject *map, PyDictObject *ret)
{
    dirty_mng_t *mng = map->dirty_mng;
    dirty_node_t *dirty_node = mng->dirty_node;

    PyObject *child = get_node_from_dirty_manager(mng, ret);

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &value)) {
        PyDict_SetItem(child, key, value);
    }
}

static void get_arr_dirty_info(PyDirtyListObject *arr, PyDictObject *ret)
{
    dirty_mng_t *mng = arr->dirty_mng;
    dirty_node_t *dirty_node = mng->dirty_node;

    PyObject *child = get_node_from_dirty_manager(mng, ret);
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &value)) {
        PyDict_SetItem(child, key, value);
    }
}

PyObject *get_dirty_info(PyObject *v)
{
    dirty_mng_t *mng = NULL;
    dirty_root_t *dirty_root;
    dirty_node_t *dirty_node; 
    PyObject *node;
    PyDictObject *ret = (PyDictObject *)PyDict_New();

    if (PyDirtyDict_CheckExact(v)) {
        mng = ((PyDirtyDictObject *)v)->dirty_mng;
    } else if (PyDirtyList_CheckExact(v)) {
        mng = ((PyDirtyListObject *)v)->dirty_mng;
    } else {
        fprintf(stderr, "%s:%d get_dirty_info fail, not support type\n", __FILE__, __LINE__);
        return Py_None;
    }

    if (!mng) {
        fprintf(stderr, "%s:%d get_dirty_info fail, not enable dirty\n", __FILE__, __LINE__);
        return Py_None;
    }

    dirty_root = mng->dirty_root;

    TAILQ_FOREACH(dirty_node, &dirty_root->dirty_node_list, entry) {
        node = dirty_node->mng->self;
        if (PyDirtyDict_CheckExact(node)) {
            printf("get map dirty info node=%p\n", node);
            get_map_dirty_info((PyDirtyDictObject *)node, ret);
            printf("get map dirty info2 node=%p\n", node);
        } else if (PyDirtyList_CheckExact(node)) {
            printf("get arr dirty info node=%p\n", node);
            get_arr_dirty_info((PyDirtyListObject *)node, ret);
            printf("get arr dirty info2 node=%p\n", node);
        }
    }

    return (PyObject *)ret;
}

PyObject *dump_dirty_info(PyObject *v)
{
    dirty_mng_t *mng = NULL;
    dirty_root_t *dirty_root;
    dirty_node_t *dirty_node; 
    PyObject *node;

    if (PyDirtyDict_CheckExact(v)) {
        mng = ((PyDirtyDictObject *)v)->dirty_mng;
    } else if (PyDirtyList_CheckExact(v)) {
        mng = ((PyDirtyListObject *)v)->dirty_mng;
    } else {
        fprintf(stderr, "%s:%d get_dirty_info fail, not support type\n", __FILE__, __LINE__);
        return Py_None;
    }

    if (!mng) {
        fprintf(stderr, "%s:%d get_dirty_info fail, not enable dirty\n", __FILE__, __LINE__);
        return Py_None;
    }

    dirty_root = mng->dirty_root;

    TAILQ_FOREACH(dirty_node, &dirty_root->dirty_node_list, entry) {
        node = dirty_node->mng->self;

        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next((PyObject *)dirty_node->dirty_key_dict, &pos, &key, &value)) {
            if (PyLong_CheckExact(key)) {
                printf("dirty info node=%p\n,key=%ld,op=%ld\n", node, PyLong_AsLong(key), PyLong_AsLong(value));
            }
            else if (PyUnicode_CheckExact(key)) {
                printf("dirty info node=%p\n,key=%s,op=%ld\n", node, PyUnicode_AS_DATA(key), PyLong_AsLong(value));
            }
        }
    }
    return NULL;
}

static void delfunc(gpointer data, gpointer user_data)
{
    printf("miss dirty subdocs key=%s\n", (char *)data);
    PyDict_DelItem((PyObject *)user_data, PyUnicode_FromString((const char *)data));
}

void handle_old_dirty_subdocs(PyDirtyDictObject *dict)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyDictObject *subdocs = dict->subdocs;

    //把不在子文档中的数据从脏数据清除
    GList *dellist = NULL;
    while (PyDict_Next((PyObject *)dict, &pos, &key, &value)) {
        if (PyDict_Contains((PyObject *)subdocs, key) == 0) {
            dellist = g_list_append(dellist, PyUnicode_AsUTF8(key));
        }
    }

    g_list_foreach(dellist, delfunc, (gpointer)dict); 
    g_list_free(dellist);
}

void handle_new_dirty_subdocs(PyDirtyDictObject *dict)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyDictObject *subdocs = dict->subdocs;

    //把所有子文档添加上
    while (PyDict_Next((PyObject *)subdocs, &pos, &key, &value)) {
        if (PyDict_Contains((PyObject *)dict, key) == 0) {
            PyDirtyDictObject *ob = PyDirtyDict_New();
            if (ob) {
                begin_dirty_manage_dict(ob, (PyObject *)dict, key);
                set_dirty_dict(dict, key, DIRTY_ADD_OP);
            }
        }
    }
}

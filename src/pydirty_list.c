#include <Python.h>
#include <structmember.h>

#include "pydirty.h"
#include "dirty.h"

static int
list_ass_subscript(PyDirtyListObject *self, PyObject *item, PyObject *value)
{
    printf("I have hack list, bingo\n");
    if (NULL != value) {
        if (!SUPPORT_DIRTY_VALUE_TYPE(value)) {
            return -1;
        }

        dirty_mng_t *vmng = NULL;
        if (PyDirtyDict_CheckExact(value)) {
            vmng = ((PyDirtyDictObject *)value)->dirty_mng;
        } else if (PyDirtyList_CheckExact(value)) {
            vmng = ((PyDirtyListObject *)value)->dirty_mng;
        }

        if (vmng != NULL && IS_DIRTY_ROOT(vmng)) {
            PyErr_Format(PyExc_TypeError, "%s:%d can not assign dirty_root to another node", __FILE__, __LINE__);
            return -1;
        }
    }

    if (!self->dirty_mng) {
        return PyList_Type.tp_as_mapping->mp_ass_subscript((PyObject *)self, item, value);
    }

    if (!PyIndex_Check(item)) {
        // TODO: slice很复杂，暂不支持，只支持整形key
        PyErr_Format(PyExc_TypeError, "%s:%d %s not support %s operate", __FILE__, __LINE__, Py_TYPE(self)->tp_name, Py_TYPE(item)->tp_name);
        return -1;
    }

    int ret = PyList_Type.tp_as_mapping->mp_ass_subscript((PyObject *)self, item, value);
    if (ret != 0) return ret;

    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
    if (i < 0) i += PyList_GET_SIZE(self);
    set_dirty_list(self, PyLong_FromLong(i), DIRTY_SET_OP);

    return ret;
}

/*
static int
list_ass_slice(PyListObject *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support '%s' operate", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "slice");
    return -1;
}
*/


static int
PyDirtyListObject_init(PyDirtyListObject *self, PyObject *args, PyObject *kwds)
{
    if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;

    self->dirty_mng = NULL;
    //begin_dirty_manage_array(self, NULL, NULL);
    return 0;
}

static void PyDirtyListObject_dealloc(PyDirtyListObject* self)
{
    free_dirty_arr_recurse(self);
    PyList_Type.tp_dealloc((PyObject *)self);
}

PyTypeObject PyDirtyList_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
        "DirtyList",         /* tp_name */
    sizeof(PyDirtyListObject),          /* tp_basicsize */
    0,                       /* tp_itemsize */
    (destructor)PyDirtyListObject_dealloc,                       /* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_compare */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    0,                       /* tp_as_sequence */
    0,                       /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    0,                       /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    //Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    //    Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DICT_SUBCLASS,         /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    0,          /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)PyDirtyListObject_init,   /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
    0,                       /* tp_free */
    0,                       /* tp_is_gc */
    0,                       /* tp_bases */
    0,                       /* tp_mro */
    0,                      /* tp_cache */
    0,                      /* tp_subclasses */
    0,                      /* tp_weaklist */
    0,                      /* tp_del */
    0,                      /* tp_version_tag */
    0,                      /* tp_finalize */
};

PyDirtyListObject *PyDirtyList_New(Py_ssize_t size)
{
    size_t nbytes;
    PyDirtyListObject *ob;
    PyListObject *listob;

    ob = (PyDirtyListObject *)PyObject_CallObject((PyObject *)&PyDirtyList_Type, NULL);
    if (ob == NULL) return NULL;
    listob = (PyListObject *)ob;

    if (size > 0) {
        nbytes = size * sizeof(PyObject *);
        listob->ob_item = (PyObject **) PyMem_Malloc(nbytes);
        if (listob->ob_item == NULL) {
            Py_DECREF(ob);
            PyErr_NoMemory();
            return NULL;
        }
        memset(listob->ob_item, 0, nbytes);
        Py_SIZE(listob) = size;
        listob->allocated = size;
    }

    return ob;
}

// 原始的list method
static PyCFunction src_listappend = NULL;
static PyCFunction src_listinsert = NULL;
static PyCFunction src_listextend = NULL;
static PyCFunction src_listpop = NULL;
static PyCFunction src_listremove = NULL;
static PyCFunction src_listreverse = NULL;
static PyCFunction src_listsort = NULL;


static PyObject *
listappend(PyDirtyListObject *self, PyObject *v)
{
/*
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "append");
    return NULL;
    */
    printf("list  append\n");
    if (!SUPPORT_DIRTY_VALUE_TYPE(v)) {
        return NULL;
    }

    PyObject *ret = src_listappend((PyObject *)self, v);
    if (ret != NULL) {
        set_dirty_list(self, PyLong_FromSsize_t(PyList_GET_SIZE(self) - 1), DIRTY_ADD_OP);
    }
    return ret;
}

static PyObject *
listinsert(PyDirtyListObject *self, PyObject *args)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "append");
    return NULL;
    /*
       Py_ssize_t where, i;
       PyObject *v;                                           
       if (!PyArg_ParseTuple(args, "nO:insert", &where, &v))      
       return NULL;                                       

       CHECK_SUPPORT_VALUE_TYPE_OR_RETURN(v, NULL)

       Py_ssize_t n = PyList_GET_SIZE(self);
       if (where < 0) {               
       where += n;                
       if (where < 0)             
       where = 0;                                     
       }
       if (where > n)                                         
       where = n;

       PyObject *ret = src_listinsert((PyObject *)self, args);
       if (ret != NULL) {
       n = PyList_GET_SIZE(self);
       if (where < n) {
    // 把where后面的设置脏
    for(i = where; i < n; ++i) {
    if (i == n - 1) {
    // 最后一个下标设置为DIRTY_ADD
    set_dirty_arr(self, i, DIRTY_ADD);
    } else {
    // 其余的设置为DIRTY_SET
    set_dirty_arr(self, i, DIRTY_SET);
    }
    }
    }
    }
    return ret;
    */
}

static PyObject *
listextend(PyListObject *self, PyObject *b)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "append");
    return NULL;
    /*
       CHECK_SUPPORT_VALUE_TYPE_OR_RETURN(b, NULL)

       Py_ssize_t self_len, b_len, begin, end;
       self_len = PyList_GET_SIZE(self);
       PyObject *ret = src_listextend((PyObject *)self, b);

       if (ret != NULL) {
       b_len = PyList_GET_SIZE(b);
       if (b_len > 0) {
    // 把新增的下标设置方DIRTY_ADD
    end = self_len + b_len;
    for (begin = self_len; begin < end; ++begin) {
    set_dirty_arr((PyDirtyListObject *)self, begin, DIRTY_ADD);
    }
    }
    }

    return ret;
    */
}

static PyObject *
listpop(PyListObject *self, PyObject *args)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "append");
    return NULL;
    /*
       Py_ssize_t pos = -1, n, i;
       PyObject *v;

       if (!PyArg_ParseTuple(args, "|n:pop", &pos)) 
       return NULL;

       v = src_listpop((PyObject *)self, args);
       if (v != NULL) {
       n = PyList_GET_SIZE(self);
       PyArg_ParseTuple(args, "|n:pop", &pos);

       if (pos < 0) {
       pos += n + 1;
       }

    // [pos, n - 2] = DIRTY_SET
    for (i = pos; i <= n - 1; ++i) {
    set_dirty_arr((PyDirtyListObject *)self, i, DIRTY_SET);
    }

    // 原来数组的最后一个设置为DIRTY_DEL
    set_dirty_arr((PyDirtyListObject *)self, n, DIRTY_DEL);
    }

    return v;
    */
}

static PyObject *
listremove(PyListObject *self, PyObject *v)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(self)->tp_name, "append");
    return NULL;
    /*
       CHECK_SUPPORT_VALUE_TYPE_OR_RETURN(v, NULL)

    // remove的要先执行脏数据记录，再实际操作remove
    Py_ssize_t i, j, n = PyList_GET_SIZE(self);

    for (i = 0; i < n; i++) {
    int cmp = PyObject_RichCompareBool(self->ob_item[i], v, Py_EQ);
    if (cmp > 0) {
    for (j = i; j < n - 1; ++j) {
    set_dirty_arr((PyDirtyListObject *)self, j, DIRTY_SET);
    }
    set_dirty_arr((PyDirtyListObject *)self, n - 1, DIRTY_DEL);
    break;
    }
    else if (cmp < 0) {
    return NULL;
    }
    }

    return src_listremove((PyObject *)self, v);
    */
}

static PyObject *
listreverse(PyListObject *self)
{
    Py_ssize_t i, n, j;
    PyObject *ret = src_listreverse((PyObject *)self, NULL);

    if (ret != NULL) {
        n = PyList_GET_SIZE(self);
        if (n > 1) {
            if (n % 2 == 0) {
                // 偶数，所有都设置为DIRTY_SET
                for (i = 0; i < n; ++i) {
                    set_dirty_list((PyDirtyListObject *)self, PyLong_FromLong(i), DIRTY_SET_OP);
                }
            } else {
                // 奇数，中间的元素不需要设置dirty
                j = n / 2;
                for (i = 0; i < n; ++i) {
                    if (i != j) {
                        set_dirty_list((PyDirtyListObject *)self, PyLong_FromLong(i), DIRTY_SET_OP);
                    }
                }
            }
        }
    }

    return ret;
}

static PyObject *
listsort(PyListObject *self, PyObject *args, PyObject *kwds)
{ 
    //printf("hack sort\n");
    Py_ssize_t i, n;
    PyObject *ret = (*(PyCFunctionWithKeywords)src_listsort)((PyObject *)self, args, kwds);
    if (ret != NULL) {
        // 简单粗暴地全部设置DIRTY_SET
        n = PyList_GET_SIZE(self);
        for (i = 0; i < n; ++i) {
            set_dirty_list((PyDirtyListObject *)self, PyLong_FromLong(i), DIRTY_SET_OP);
        }
    }
    return ret;
}

void init_dirty_list(void)
{
    PyDirtyList_Type.tp_base = &PyList_Type;

    // hack tp_as_mapping
    PyMappingMethods *list_as_mapping = (PyMappingMethods *)malloc(sizeof(PyMappingMethods));
    memcpy(list_as_mapping, PyList_Type.tp_as_mapping, sizeof(PyMappingMethods));
    PyDirtyList_Type.tp_as_mapping = list_as_mapping;
    PyDirtyList_Type.tp_as_mapping->mp_ass_subscript = (objobjargproc)list_ass_subscript;

    // hack tp_as_sequence
    PySequenceMethods *list_as_sequence = (PySequenceMethods *)malloc(sizeof(PySequenceMethods));
    memcpy(list_as_sequence, PyList_Type.tp_as_sequence, sizeof(PySequenceMethods));
    PyDirtyList_Type.tp_as_sequence = list_as_sequence;
    //PyDirtyList_Type.tp_as_sequence->sq_ass_slice = (ssizessizeobjargproc)list_ass_slice;

    int c = 1;
    PyMethodDef *def = PyList_Type.tp_methods;
    while(def->ml_name != NULL) {
        c++;
        def++;
    }

    PyMethodDef *list_methods = (PyMethodDef *)malloc(sizeof(PyMethodDef) * c);
    memset(list_methods, 0, sizeof(PyMethodDef) * c);
    memcpy(list_methods, PyList_Type.tp_methods, sizeof(PyMethodDef) * c);
    PyDirtyList_Type.tp_methods = list_methods;

    def = list_methods;
    while(def->ml_name != NULL) {
        if (strcmp(def->ml_name, "append") == 0) {
            src_listappend = def->ml_meth;
            def->ml_meth = (PyCFunction)listappend;
        } else if (strcmp(def->ml_name, "insert") == 0) {
            src_listinsert = def->ml_meth;
            def->ml_meth = (PyCFunction)listinsert;
        } else if (strcmp(def->ml_name, "extend") == 0) {
            src_listextend = def->ml_meth;
            def->ml_meth = (PyCFunction)listextend;
        } else if (strcmp(def->ml_name, "pop") == 0) {
            src_listpop = def->ml_meth;
            def->ml_meth = (PyCFunction)listpop;
        } else if (strcmp(def->ml_name, "remove") == 0) {
            src_listremove = def->ml_meth;
            def->ml_meth = (PyCFunction)listremove;
        } else if (strcmp(def->ml_name, "reverse") == 0) {
            src_listreverse = def->ml_meth;
            def->ml_meth = (PyCFunction)listreverse;
        } else if (strcmp(def->ml_name, "sort") == 0) {
            src_listsort = def->ml_meth;
            def->ml_meth = (PyCFunction)listsort;
        }

        def++;
    }
}

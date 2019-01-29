#include <Python.h>
#include <stdio.h>
#include "dirty.h"
#include "pydirty.h"

extern PyObject *DBError;

static int PyDirtyDictObject_init(PyDirtyDictObject *self, PyObject *args, PyObject *kwds)
{
    //printf("on init\n");
    if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->dirty_mng = NULL;
    self->subdocs = NULL;
    //printf("on init success\n");
    return 0;
}

static void PyDirtyDictObject_dealloc(PyDirtyDictObject* self)
{
    printf("on dealloc show val .\n");
    PyObject_Print((PyObject *)self, stdout, 0);
    printf("\n");
    free_dirty_dict_recurse(self);
    PyDict_Type.tp_dealloc((PyObject *)self);
}

static int dirty_dict_ass_sub(PyDirtyDictObject *self, PyObject *key, PyObject *val)
{
    printf("on dirty_dict_ass_sub\n");
    int ret;
    PyObject *new = NULL;

    MY_PYOBJECT_PRINT(self, "on dirty_dict_ass_sub");
    MY_PYOBJECT_PRINT(key, "on dirty_dict_ass_sub");
    MY_PYOBJECT_PRINT(val, "on dirty_dict_ass_sub");

    int contain = PyDict_Contains((PyObject *)self, key);
    if (!PyLong_CheckExact(key) && !PyUnicode_CheckExact(key)) {
        PyErr_Format(PyExc_TypeError, "%s:%d key must be int or string.", __FILE__, __LINE__);
        return -1;
    }

    //为了安全根节点有一些操作不允许
    //1、设置的key不在顶层子文档列表中不允许
    //2、删除顶层子文档列表中的key不允许
    if (IS_DIRTY_ROOT(self->dirty_mng)) {
        dirty_root_t *dirty_root = self->dirty_mng->dirty_root;
        assert(dirty_root != NULL && self->subdocs != NULL);
        if (PyDict_Contains(self->subdocs, key)) {
            if (val == NULL) {
                PyErr_Format(DBError, "%s:%d forbid del top subdocs key. key : %s", __FILE__, __LINE__, PyUnicode_AsUTF8(PyObject_Repr(key)));
                return -1;
            }
        } else {
            PyErr_Format(DBError, "%s:%d forbid set top key which not in subdocs. key : %s", __FILE__, __LINE__, PyUnicode_AsUTF8(PyObject_Repr(key)));
            return -1;
        }
    }

    if (val == NULL) {
        if (!contain) {
            return 0;
        } else {
            set_dirty_dict(self, key, DIRTY_DEL_OP);
            return PyDict_DelItem((PyObject *)self, key);
        }
    } else {
        if (!SUPPORT_DIRTY_VALUE_TYPE(val)) {
            PyErr_Format(PyExc_TypeError, "%s:%d unsupported value type. type : %s", __FILE__, __LINE__, Py_TYPE(val)->tp_name);
            return -1;
        }

        if (PyDict_CheckExact(val)) {
            new = (PyObject *)build_dirty_dict((PyDictObject *)val);
            begin_dirty_manage_dict((PyDirtyDictObject *)new, (PyObject *)self, key);
        } else if (PyList_CheckExact(val)) {
            new = (PyObject *)build_dirty_list((PyListObject *)val);
            begin_dirty_manage_list((PyDirtyListObject *)new, (PyObject *)self, key);
        } else {
            new = val;
        }

        ret = PyDict_SetItem((PyObject *)self, key, new);

        MY_PYOBJECT_PRINT(self, "on dirty_dict_ass_sub set");
        MY_PYOBJECT_PRINT(key, "on dirty_dict_ass_sub set");
        MY_PYOBJECT_PRINT(val, "on dirty_dict_ass_sub set");

        if (ret != 0) return ret;
        if (!contain) {
            set_dirty_dict(self, key, DIRTY_ADD_OP);
        } else {
            set_dirty_dict(self, key, DIRTY_SET_OP);
        }
    }
    
    return ret;
}

static PyObject * dirty_dict_repr(PyDirtyDictObject *mp)
{
    //只是拷贝Python dict object显示代码 ^_^
    //-------------------------------------------------
    Py_ssize_t i;
    PyObject *key = NULL, *value = NULL;
    _PyUnicodeWriter writer;
    int first;

    i = Py_ReprEnter((PyObject *)mp);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("{...}") : NULL;
    }

    if (((PyDictObject *)mp)->ma_used == 0) {
        Py_ReprLeave((PyObject *)mp);
        return PyUnicode_FromString("{}");
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    /* "{" + "1: 2" + ", 3: 4" * (len - 1) + "}" */
    writer.min_length = 1 + 4 + (2 + 4) * (((PyDictObject*)mp)->ma_used - 1) + 1;

    if (_PyUnicodeWriter_WriteChar(&writer, '{') < 0)
        goto error;

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    i = 0;
    first = 1;
    while (PyDict_Next((PyObject *)mp, &i, &key, &value)) {
        PyObject *s;
        int res;

        /* Prevent repr from deleting key or value during key format. */
        Py_INCREF(key);
        Py_INCREF(value);

        if (!first) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }
        first = 0;

        s = PyObject_Repr(key);
        if (s == NULL)
            goto error;
        res = _PyUnicodeWriter_WriteStr(&writer, s);
        Py_DECREF(s);
        if (res < 0)
            goto error;

        if (_PyUnicodeWriter_WriteASCIIString(&writer, ": ", 2) < 0)
            goto error;

        s = PyObject_Repr(value);
        if (s == NULL)
            goto error;
        res = _PyUnicodeWriter_WriteStr(&writer, s);
        Py_DECREF(s);
        if (res < 0)
            goto error;

        Py_CLEAR(key);
        Py_CLEAR(value);
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, '}') < 0)
        goto error;

    //-------------------------------------------------
    //show dirty info start
    writer.overallocate = 1;
    

    Py_ReprLeave((PyObject *)mp);

    return _PyUnicodeWriter_Finish(&writer);

error:
    Py_ReprLeave((PyObject *)mp);
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(key);
    Py_XDECREF(value);
    return NULL;
}

PyTypeObject PyDirtyDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dirty_dict",             /* tp_name */
    sizeof(PyDirtyDictObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyDirtyDictObject_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    (reprfunc)dirty_dict_repr,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|        /* tp_flags */
       Py_TPFLAGS_BASETYPE, 
    "Dirty Dict Object",           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    0,                       /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)PyDirtyDictObject_init,   /* tp_init */
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

PyDirtyDictObject *PyDirtyDict_New()
{
    return (PyDirtyDictObject *)PyObject_CallObject((PyObject *)&PyDirtyDict_Type, NULL);
}

static PyCFunction original_dict_pop = NULL;
static PyObject *dirty_dict_pop(PyDictObject *mp, PyObject *args)
{
    printf("on dict_pop\n");
    PyObject *key, *deflt = NULL;

    if(!PyArg_UnpackTuple(args, "pop", 1, 2, &key, &deflt))
        return NULL;

    if (PyDict_GetItem((PyObject *)mp, key)) {
        set_dirty_dict((PyDirtyDictObject *)mp, key, DIRTY_DEL_OP);
    }

    return original_dict_pop((PyObject *)mp, args);
}
    
static PyCFunction original_dict_popitem = NULL;
static PyObject *dirty_dict_popitem(PyDictObject *mp)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(mp)->tp_name, "popitem");
    return NULL;
}

static PyCFunction original_dict_clear = NULL;
static PyObject *dirty_dict_clear(PyDictObject *dict)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next((PyObject *)dict, &pos, &key, &value)) {
        set_dirty_dict((PyDirtyDictObject *)dict, key, DIRTY_DEL_OP);
    }
    return original_dict_clear((PyObject *)dict, NULL);
}

void init_dirty_dict(void)
{
    PyDirtyDict_Type.tp_base = &PyDict_Type;

    //覆盖dict原版映射赋值
    PyMappingMethods *dict_as_mapping = (PyMappingMethods *)malloc(sizeof(PyMappingMethods));
    memcpy(dict_as_mapping, PyDict_Type.tp_as_mapping, sizeof(PyMappingMethods));
    PyDirtyDict_Type.tp_as_mapping = dict_as_mapping;
    PyDirtyDict_Type.tp_as_mapping->mp_ass_subscript = (objobjargproc)dirty_dict_ass_sub;

    int count = 1;
    for (PyMethodDef *method = PyDict_Type.tp_methods; method->ml_name != NULL; method++) count++;
    
    PyMethodDef *dict_methods = (PyMethodDef *)malloc(sizeof(PyMethodDef) * count); 
    memset(dict_methods, 0, sizeof(PyMethodDef) * count); 
    memcpy(dict_methods, PyDict_Type.tp_methods, sizeof(PyMethodDef) * count);
    PyDirtyDict_Type.tp_methods = dict_methods;

    //覆盖dict原版字典方法
    for (PyMethodDef *method = PyDirtyDict_Type.tp_methods; method->ml_name != NULL; method++) {
        if (strcmp(method->ml_name, "pop") == 0) {
            original_dict_pop = method->ml_meth;
            method->ml_meth = (PyCFunction)dirty_dict_pop;
        }
        else if (strcmp(method->ml_name, "popitem") == 0) {
            original_dict_popitem = method->ml_meth;
            method->ml_meth = (PyCFunction)dirty_dict_popitem;
        }
        else if (strcmp(method->ml_name, "clear") == 0) {
            original_dict_clear = method->ml_meth;
            method->ml_meth = (PyCFunction)dirty_dict_clear;
        }
    }

    printf("on init_dirty_dict\n");
}

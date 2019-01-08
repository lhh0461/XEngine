#include <Python.h>
#include <stdio.h>
#include "dirty.h"
#include "pydirty.h"

static int PyDirtyDictObject_init(PyDirtyDictObject *self, PyObject *args, PyObject *kwds)
{
    printf("on init\n");
    if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->dirty_mng = NULL;
    printf("on init success\n");
    return 0;
}

static void PyDirtyDictObject_dealloc(PyDirtyDictObject* self)
{
    printf("on dealloc\n");
    PyDict_Type.tp_dealloc((PyObject *)self);
    clear_dirty((PyObject *)self);
}

static int dirty_dict_ass_sub(PyDirtyDictObject *self, PyObject *key, PyObject *val)
{
    printf("on dirty_dict_ass_sub\n");
    int ret;

    int contain = PyDict_Contains((PyObject *)self, key);
    if (val == NULL) {
        if (!contain) {
            return 0;
        } else {
            set_dirty_dict(self, key, DIRTY_DEL_OP);
            return PyDict_Type.tp_as_mapping->mp_ass_subscript((PyObject *)self, key, val);
        }
    } else {
        ret = PyDict_Type.tp_as_mapping->mp_ass_subscript((PyObject *)self, key, val);
        if (ret != 0) return ret;
        set_dirty_dict(self, key, DIRTY_SET_OP);
    }
    
    return ret;
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
    0,                         /* tp_repr */
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
    "Dirty Save objects",           /* tp_doc */
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
};

static PyCFunction original_dict_pop = NULL;
static PyObject *
dirty_dict_pop(PyDictObject *mp, PyObject *args)
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
static PyObject *
dirty_dict_popitem(PyDictObject *mp)
{
    PyErr_Format(PyExc_TypeError, "%s:%d %s not support method '%s'", __FILE__, __LINE__, Py_TYPE(mp)->tp_name, "popitem");
    return NULL;
    /*
    printf("on dict_popitem\n");

    PyObject *popitem = original_dict_popitem((PyObject *)mp, NULL);

    printf("on dict_popitem end1\n");
    if (PyTuple_CheckExact(popitem)) {
        printf("on dict_popitem end2\n");
        PyObject *key, *value;
        printf("on dict_popitem end3\n");
        if (PyArg_ParseTuple(popitem, "OO", &key, &value)) {
            printf("on dict_popitem end4\n");
            printf("on dict_popitem end6\n");
            set_dirty_dict((PyDirtyDictObject *)mp, key, DIRTY_DEL_OP);
        }
    }

    printf("on dict_popitem end5\n");
    return popitem;
    */
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
            
        }
    }

    printf("on init_dirty_dict\n");
}

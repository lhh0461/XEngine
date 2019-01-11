#include <Python.h>
#include <stdio.h>
#include "dirty.h"
#include "pydirty.h"

PyDirtyDictObject *build_dirty_dict(PyDictObject *dict)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    PyDirtyDictObject *ob = (PyDirtyDictObject *)PyObject_CallObject((PyObject *)&PyDirtyDict_Type, NULL);
    PyObject *new = NULL;

    while (PyDict_Next((PyObject *)dict, &pos, &key, &value)) {
        
        if (PyString_CheckExact(value) || 
            PyFloat_CheckExact(value) ||
            PyInt_CheckExact(value) ||
            PyLong_CheckExact(value) ||
            PyFloat_CheckExact(value))
        {
            Py_INCREF(value);
            new = value;
        } else if (PyDict_CheckExact(value)) {
            new = (PyObject *)build_dirty_dict((PyDictObject *)value);
        }
        if (PyList_CheckExact(value)) {
            new = (PyObject *)build_dirty_list((PyListObject *)value);
        }
        if (new == NULL) {
            Py_DECREF(ob);
            return NULL;
        }
            
        if (PyDict_SetItem((PyObject *)ob, key, new) == 0) {
            Py_DECREF(new);
        }
    }
    return ob;
}

PyDirtyListObject * build_dirty_list(PyListObject *list)
{
    PyObject *value;
    Py_ssize_t pos = 0;

    PyDirtyListObject *ob;
    PyObject *new = NULL;

    Py_ssize_t size = PyList_GET_SIZE((PyObject *)list);
    ob = PyDirtyList_New(size);
    if (ob == NULL) return NULL;

    for (; pos < size; pos++) {
        value = PyList_GET_ITEM((PyObject *)list, pos);
        if (PyString_CheckExact(value) || 
            PyFloat_CheckExact(value) ||
            PyInt_CheckExact(value) ||
            PyLong_CheckExact(value)) 
        {
            Py_INCREF(value);
            new = value;
        } else if (PyDict_CheckExact(value)) {
            new = (PyObject *)build_dirty_dict((PyDictObject *)value);
        }
        if (PyList_CheckExact(value)) {
            new = (PyObject *)build_dirty_list((PyListObject *)value);
        }
        if (new == NULL) {
            Py_DECREF(ob);
            return NULL;
        }
            
        PyList_SET_ITEM((PyObject *)ob, pos, value);
    }
    return ob;
}

static PyObject *restore(PyObject *self, PyObject *args)
{
    PyObject *savename;
    if (!PyArg_ParseTuple(args, "S", &savename)) {
        return NULL;
    }
    if (!PyString_CheckExact(savename)) {
        return NULL;
    }
    //TODO check is load?
   
    PyDirtyDictObject *dict = build_dirty_dict((PyDictObject *)Py_BuildValue("{i:[i,i],i:i}", 1, 3, 3, 2, 2));
    begin_dirty_manage_dict(dict, NULL, NULL);

    return (PyObject *)dict;
}

static PyObject *unload(PyObject *self, PyObject *args)
{
    
    Py_RETURN_TRUE;
}

static PyObject *pydirty_get_dirty_info(PyObject *self, PyObject *args)
{
    PyObject *v; 

    if (!PyArg_ParseTuple(args, "O", &v)) {
        return NULL;
    }   

    //CHECK_DIRTY_TYPE_OR_RETURN(v, NULL)

    PyObject *ret = get_dirty_info(v);
    return Py_BuildValue("N", ret);
}

static PyObject *pydump_dirty_info(PyObject *self, PyObject *args)
{
    PyObject *v; 

    if (!PyArg_ParseTuple(args, "O", &v)) {
        return NULL;
    }   

    //CHECK_DIRTY_TYPE_OR_RETURN(v, NULL)

    PyObject *ret = dump_dirty_info(v);
    return Py_None;
}

static PyObject *serialize(PyObject *self, PyObject *args)
{
    PyObject *v; 

    if (!PyArg_ParseTuple(args, "O", &v)) {
        return NULL;
    }   

    if (!PyDirtyDict_CheckExact(v)) {
        return NULL;
    }

    PyObject *ret = dump_dirty_info(v);
    return Py_None;
}

static PyMethodDef dirtytype_methods[] = {
    {"restore", (PyCFunction)restore, METH_VARARGS, "restore save data from db"},
    {"save", (PyCFunction)restore, METH_VARARGS, "restore save data from db"},
    {"unload", (PyCFunction)unload, METH_O, "store save data to db and release"},
    {"serialize", (PyCFunction)unload, METH_O, "serialize data"},
    {"serialize_dirty", (PyCFunction)unload, METH_O, "serialize dirty"},
    {"get_dirty_info", (PyCFunction)pydump_dirty_info, METH_VARARGS, "store save data to db and release"},
    {NULL}  /* Sentinel */
};

PyMODINIT_FUNC initdirty(void) 
{
    PyObject* m;

    init_dirty_dict();
    init_dirty_list();

    if (PyType_Ready(&PyDirtyDict_Type) < 0)
        return;
    if (PyType_Ready(&PyDirtyList_Type) < 0)
        return;

    m = Py_InitModule3("dirty", dirtytype_methods,
            "This is dirty module.");
    if (m == NULL) {
        return;
    }

    Py_INCREF(&PyDirtyDict_Type);
    Py_INCREF(&PyDirtyList_Type);

    PyModule_AddObject(m, "dirty_dict", (PyObject *)&PyDirtyDict_Type);
    PyModule_AddObject(m, "dirty_list", (PyObject *)&PyDirtyList_Type);

    dirty_mem_pool_setup();
}

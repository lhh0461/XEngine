#include <Python.h>
#include <stdio.h>
#include "dirty.h"
#include "pydirty.h"

//static PyDirtyListObject * build_dirty_list(PyListObject *list)
//{
//    return NULL;
//}

static PyDirtyDictObject *build_dirty_dict(PyDictObject *dict)
{
    
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    PyDirtyDictObject *ob = (PyDirtyDictObject *)PyObject_CallObject((PyObject *)&PyDirtyDict_Type, NULL);
    PyObject *new = NULL;

    while (PyDict_Next((PyObject *)dict, &pos, &key, &value)) {
        
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
        //if (PyList_CheckExact(value)) {
        //    new = build_dirty_list(value);
        //}
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

static PyObject *load(PyObject *self, PyObject *args)
{
    PyObject *savename;
    if (!PyArg_ParseTuple(args, "S", &savename)) {
        return NULL;
    }
    if (!PyString_CheckExact(savename)) {
        return NULL;
    }
    //TODO check is load?
   
    PyDirtyDictObject *dict = build_dirty_dict((PyDictObject *)Py_BuildValue("{i:{i:i},i:i}", 1, 3, 3, 2, 2));
    begin_dirty_manage_dict(dict, NULL, NULL);

    return (PyObject *)dict;
}

static PyObject *unload(PyObject *self, PyObject *args)
{
    
    Py_RETURN_TRUE;
}

static PyMethodDef dirtytype_methods[] = {
    {"load", (PyCFunction)load, METH_VARARGS, "load save data from db"},
    {"unload", (PyCFunction)unload, METH_O, "store save data to db and release"},
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

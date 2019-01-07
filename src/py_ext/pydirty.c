#include <Python.h>
#include <stdio.h>
#include "dirty.h"
#include "pydirty.h"

//static PyDirtyListObject * build_dirty_list(PyListObject *list)
//{
//    Py_RETURN_TRUE;
//}
//
//static PyDirtyDictObject * build_dirty_dict(PyDictObject *dict)
//{
//    Py_RETURN_TRUE;
//}

static PyObject *load(PyObject *self, PyObject *args)
{
    PyObject *ob;
    PyObject *savename;
    if (!PyArg_ParseTuple(args, "OS", &ob, &savename)) {
        return NULL;
    }
    if (!PyString_CheckExact(savename))
        return NULL;
    if (PyDirtyDict_CheckExact(ob)) {
        PyDictObject *dictOb = (PyDictObject *)ob;
        PyMapping_SetItemString((PyObject *)dictOb, PyString_AsString(savename), Py_BuildValue("{i:i}", 1, 1));
        //enable_dirty();
    }
    Py_RETURN_TRUE;
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

    if (PyType_Ready(&PyDirtyDict_Type) < 0)
        return;

    m = Py_InitModule3("dirty", dirtytype_methods,
            "This is dirty module.");

    if (m == NULL) {
        return;
    }

    Py_INCREF(&PyDirtyDict_Type);
    PyModule_AddObject(m, "dirty_dict", (PyObject *)&PyDirtyDict_Type);
}

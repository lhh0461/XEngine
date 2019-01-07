#include <Python.h>
#include "dirty.h"
#include "pydirty.h"

void set_dirty_data(PyObject *ob, PyObject *key, enum dirty_op_e op)
{
    if (PyDirtyDict_CheckExact(ob)) {
        
        
    }
    /*
    } else if (PyDirtyList_CheckExact(ob)) {

    } else {
        PyErr_Format(PyExc_TypeError, "ob is not dirty dict or dirty list");
    }
    */
}

void clean_dirty(PyObject *self)
{

}

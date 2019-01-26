#include <Python.h>
#include <stdio.h>
#include <stdio.h>
#include <glib.h>
#include "db.h"
#include "pydirty.h"
#include "dirty.h"

PyDirtyDictObject *build_dirty_dict(PyDictObject *dict)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    PyDirtyDictObject *ob = PyDirtyDict_New();
    PyObject *new = NULL;

    while (PyDict_Next((PyObject *)dict, &pos, &key, &value)) {
        
        if (PyUnicode_CheckExact(value) || 
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

    for (; pos < size; pos++)
    {
        value = PyList_GET_ITEM((PyObject *)list, pos);
        if (PyUnicode_CheckExact(value) || 
            PyFloat_CheckExact(value) ||
            PyLong_CheckExact(value)) 
        {
            Py_INCREF(value);
            new = value;
        } 
        else if (PyDict_CheckExact(value)) {
            new = (PyObject *)build_dirty_dict((PyDictObject *)value);
        }
        else if (PyList_CheckExact(value)) {
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

void set_dirty_dict_subdocs(PyDirtyDictObject *dict, PyObject *subdocs)
{
    PyObject *key, *value;
    dict->subdocs = (PyDictObject *)PyDict_New();
    Py_ssize_t size = PyTuple_GET_SIZE(subdocs);
    Py_ssize_t pos = 0;

    for (; pos < size; pos++)
    {
        value = PyTuple_GET_ITEM(subdocs, pos);
        if (!PyUnicode_CheckExact(value))  {
            //TODO EXCEPTION
        }
        Py_INCREF(value);
        if (PyDict_SetItem((PyObject *)dict->subdocs, value, PyLong_FromLong(1)) == 0) {
            Py_DECREF(value);
        }
    }
}

static PyObject *sync_load(PyObject *self, PyObject *args)
{
    const char*docname;
    int sid;
    PyObject *subdocs;

    PyObject_Print(args, stdout, 0);

    if (!PyArg_ParseTuple(args, "siO", &docname, &sid, &subdocs)) {
        return NULL;
    }
    printf("load sid=%d\n", sid);
    printf("load docname=%s\n", docname);

    if (!PyTuple_CheckExact(subdocs)) {
        //TODO EXCEPTION
        return NULL;
    }

    //TODO check is load?
    PyDictObject *obj = (PyDictObject *)load_dat_sync(docname, sid);
    if (!obj) {
        return NULL;
    }
    PyObject_Print((PyObject *)obj, stdout, 0);
    printf("print obj\n");

    PyDirtyDictObject *dict = build_dirty_dict(obj);
    set_dirty_dict_subdocs(dict, subdocs);

    handle_old_dirty_subdocs(dict);
    begin_dirty_manage_dict(dict, NULL, NULL);
    handle_new_dirty_subdocs(dict);

    return (PyObject *)dict;
}

static PyObject *unload(PyObject *self, PyObject *args)
{
    
    Py_RETURN_TRUE;
}

/*
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
*/

static PyObject *pydump_dirty_info(PyObject *self, PyObject *args)
{
    PyObject *v; 

    if (!PyArg_ParseTuple(args, "O", &v)) {
        return NULL;
    }   

    //CHECK_DIRTY_TYPE_OR_RETURN(v, NULL)

    PyObject *ret = dump_dirty_info(v);
    return ret;
}

/*
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
    return ret;
}
*/

static PyObject * async_load(PyObject *self, PyObject *args)
{
    char *doc_id;
    int sid;
    printf("async load before\n");
    if (!PyArg_ParseTuple(args, "si", &doc_id, &sid)) {
        printf("parse tuple faile\n");
        return 0;
    }

    printf("async load\n");
    //if (PyLong_CheckExact(doc_id)) {
    //    return load_user_async(PyLong_AsLong(doc_id), sid);
    //} else if (PyUnicode_CheckExact(doc_id)) {
        int ret = load_dat_async(doc_id, sid);
        return Py_BuildValue("i", ret);
    //}
}

static PyObject *save_all(PyObject *self, PyObject *args)
{
    int save_dirty;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int key_type;

    if (!PyArg_ParseTuple(args, "i", &save_dirty)) {
        return NULL;
    }

    PyObject * dbmodule = PyImport_Import("db");
    if (dbmodule == NULL) {
        return NULL;
    }

    PyObject *objdict = PyObject_GetAttrString(dbmodule, "all_db_obj");
    if (objdict == NULL) {
        return NULL;
    }

    if (!PyDict_CheckExact(objdict)) {
        return NULL;
    }
     
    while (PyDict_Next((PyObject *)objdict, &pos, &key, &value)) {
       if (PyLong_CheckExact(key)) {
           save_user(save_dirty, PyLong_AsLong(key), value);
       } else {
           save_dat(save_dirty, PyUnicode_AsUTF8(key), value);
       }
    }

    return (PyObject *)dict;
}

static PyMethodDef dirty_method[] = {
    {"sync_load", (PyCFunction)sync_Load, METH_VARARGS, "sync load db obj from db"},
    {"async_load", (PyCFunction)async_load, METH_VARARGS, "async load db obj from db"},
    {"new_doc", (PyCFunction)new_doc, METH_VARARGS, "new db obj to db"},
    {"unload", (PyCFunction)unload, METH_VARARGS, "save specific db obj to db and release it"},
    {"save_all", (PyCFunction)save_all, METH_VARARGS, "save all db obj to db"},
    {"save_one", (PyCFunction)save_one, METH_VARARGS, "save specific db obj to db"},
    {"marshal", (PyCFunction)unload, METH_O, "test marshal dirty dict to bytes"},
    {"unmarshal", (PyCFunction)unload, METH_O, "test unmarshal bytes to dirty dict"},
    {"serialize_dirty", (PyCFunction)unload, METH_O, "serialize dirty"},
    {"get_dirty_info", (PyCFunction)pydump_dirty_info, METH_VARARGS, "store save data to db and release"},
    {NULL}  /* Sentinel */
};

static struct PyModuleDef dirtymodule = {
    PyModuleDef_HEAD_INIT,
    "dirty",   /* name of module */
    "this is dirty module", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    dirty_method,
    0,
    0,
    0,
    0,
};


PyMODINIT_FUNC PyInit_dirty(void) 
{
    PyObject* m;

    init_dirty_dict();
    init_dirty_list();

    if (PyType_Ready(&PyDirtyDict_Type) < 0)
        return NULL;
    if (PyType_Ready(&PyDirtyList_Type) < 0)
        return NULL;

    m = PyModule_Create(&dirtymodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&PyDirtyDict_Type);
    Py_INCREF(&PyDirtyList_Type);

    PyModule_AddObject(m, "dirty_dict", (PyObject *)&PyDirtyDict_Type);
    PyModule_AddObject(m, "dirty_list", (PyObject *)&PyDirtyList_Type);

    return m;
}

int init_dirty_module()
{
    PyImport_AppendInittab("dirty", &PyInit_dirty);
    printf("init dirty module success!!!\n");
}

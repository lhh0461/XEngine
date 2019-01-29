#include <Python.h>
#include <stdio.h>
#include <stdio.h>
#include <glib.h>

#include "db.h"
#include "pydirty.h"
#include "dirty.h"
#include "marshal.h"
#include "buffer.h"

PyObject *DBError;

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
    dict->subdocs = PyDict_New();
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
    PyObject *docid;
    PyObject *subdocs;
    int sid;
    int state;

    if (!PyArg_ParseTuple(args, "OiO", &docid, &sid, &subdocs)) {
        return NULL;
    }

    PyDictObject *obj = (PyDictObject *)load_db_obj_sync(docid, sid, &state);
    if (state == STATE_OK) {
        if (obj == NULL) {
            PyErr_SetString(DBError, "load db object error");
            return NULL;
        }

    } else if (state == STATE_NULL) {
        obj = (PyDictObject *)new_db_object(docid);
    } else if (state == STATE_ERROR) {
        PyErr_SetString(DBError, "load db object error");
        return NULL;
    }

    PyDirtyDictObject *dict = build_dirty_dict(obj);
    if (!dict) {
        PyErr_SetString(DBError, "load db object build dirty fail");
        return NULL;
    }

    set_dirty_dict_subdocs(dict, subdocs);

    handle_old_dirty_subdocs(dict);
    begin_dirty_manage_dict(dict, NULL, NULL);
    handle_new_dirty_subdocs(dict);

    return (PyObject *)dict;
}

static PyObject * async_load(PyObject *self, PyObject *args)
{
    PyObject *docid;
    int sid;

    if (!PyArg_ParseTuple(args, "Oi", &docid, &sid)) {
        return NULL;
    }

    int ret = load_db_obj_async(docid, sid);
    return Py_BuildValue("i", ret);
}

static PyObject *unload(PyObject *self, PyObject *args)
{
    PyObject *docid;
    PyObject *obj;
    int save_method; 

    if (!PyArg_ParseTuple(args, "OOi", &docid, &obj, &save_method)) {
        return NULL;
    }

    if (save_db_obj(docid, obj, save_method) == -1) {
        PyErr_SetString(DBError, "unload db object error");
        return NULL;
    }

    unload_db_obj(docid);
    
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

static PyObject *save_all(PyObject *self, PyObject *args)
{
    int save_method;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int key_type;

    if (!PyArg_ParseTuple(args, "i", &save_method)) {
        return NULL;
    }

    PyObject * dbmodule = PyImport_ImportModule("db");
    if (dbmodule == NULL) {
        return NULL;
    }

    PyObject *objdict = PyObject_GetAttrString(dbmodule, "all_db_obj");
    if (objdict == NULL) {
        return NULL;
    }

    if (!PyDict_CheckExact(objdict)) {
        PyErr_Format(PyExc_TypeError, "all_db_obj must be dict tpye  type=%s", Py_TYPE(value)->tp_name);
        return NULL;
    }
     
    while (PyDict_Next((PyObject *)objdict, &pos, &key, &value)) {
        save_db_obj(key, value, save_method);
    }

    Py_RETURN_NONE;
}

static PyObject *new_doc(PyObject *self, PyObject *args)
{
    PyObject *docid;
    PyObject *subdocs;

    if (!PyArg_ParseTuple(args, "OO", &docid, &subdocs)) {
        return NULL;
    }

    PyDictObject *obj = (PyDictObject *)new_db_object(docid);
    if (obj == NULL) {
        return NULL;
    }

    PyDirtyDictObject *dict = build_dirty_dict(obj);
    if (!dict) {
        PyErr_SetString(DBError, "load db object build dirty fail");
        return NULL;
    }

    set_dirty_dict_subdocs(dict, subdocs);

    handle_old_dirty_subdocs(dict);
    begin_dirty_manage_dict(dict, NULL, NULL);
    handle_new_dirty_subdocs(dict);

    return (PyObject *)dict;
}

static PyObject *save_doc(PyObject *self, PyObject *args)
{
    PyObject *docid;
    PyObject *obj;
    int save_method;

    if (!PyArg_ParseTuple(args, "OOi", &docid, &obj, &save_method)) {
        return NULL;
    }

    if (!PyUnicode_CheckExact(docid) && !PyLong_CheckExact(docid)) {
        PyErr_Format(PyExc_TypeError, "save doc key must be string or int type=%s", Py_TYPE(docid)->tp_name);
        return NULL;
    }


    if (!PyDirtyDict_CheckExact(obj)) {
        PyErr_Format(PyExc_TypeError, "save doc obj must be dirty dirty type type=%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
     
    save_db_obj(docid, obj, save_method);
    Py_RETURN_NONE;
}

static PyObject *marshal_obj(PyObject *self, PyObject *args)
{
    PyObject *obj, *ret;
    marshal_array_t arr;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if (!PyDirtyDict_CheckExact(args)) {
        return NULL;
    }

    marshal_array_construct(&arr); 
    marshal_dirty(obj, &arr);

    char *bytes = buffer_pullup(&arr.buf);
    unsigned len = arr.buf.data_size;
    ret = PyUnicode_FromStringAndSize(bytes, len);
    marshal_array_destruct(&arr);

    if (ret == NULL) return NULL;
    return Py_BuildValue("N", ret);
}

static PyObject *enable_dirty(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *subdocs;

    if (!PyArg_ParseTuple(args, "OO", &obj, &subdocs)) {
        return NULL;
    }

    if (!PyDict_CheckExact(obj)) {
        PyErr_Format(PyExc_TypeError, "save doc obj must be dirty dirty type type=%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
     
    if (!PyTuple_CheckExact(subdocs)) {
        PyErr_Format(PyExc_TypeError, "save doc obj must be dirty dirty type type=%s", Py_TYPE(obj)->tp_name);
        return NULL;
    }
     
    PyDirtyDictObject *dict = build_dirty_dict((PyDictObject *)obj);
    if (!dict) {
        PyErr_SetString(DBError, "load db object build dirty fail");
        return NULL;
    }

    set_dirty_dict_subdocs(dict, subdocs);

    handle_old_dirty_subdocs(dict);
    begin_dirty_manage_dict(dict, NULL, NULL);
    handle_new_dirty_subdocs(dict);

    return (PyObject *)dict;
}

static PyMethodDef dirty_method[] = {
    {"sync_load", (PyCFunction)sync_load, METH_VARARGS, "sync load db obj from db"},
    {"async_load", (PyCFunction)async_load, METH_VARARGS, "async load db obj from db"},
    {"new_doc", (PyCFunction)new_doc, METH_VARARGS, "new db obj to db"},
    {"unload", (PyCFunction)unload, METH_VARARGS, "save specific db obj to db and release it"},
    {"save_all", (PyCFunction)save_all, METH_VARARGS, "save all db obj to db"},
    {"save_doc", (PyCFunction)save_doc, METH_VARARGS, "save one db obj to db"},
    {"enable_dirty", (PyCFunction)enable_dirty, METH_VARARGS, "save one db obj to db"},
    {"marshal", (PyCFunction)marshal_obj, METH_VARARGS, "test marshal dirty dict to bytes"},
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

    DBError = PyErr_NewException("Db.Error", NULL, NULL);
    Py_INCREF(DBError);
    PyModule_AddObject(m, "DBError", DBError);

    return m;
}

int init_dirty_module()
{
    PyImport_AppendInittab("dirty", &PyInit_dirty);
    printf("init dirty module success!!!\n");
}


#include <Python.h>

typedef struct {
    PyObject_HEAD
        /* Type-specific fields go here. */
} save_SaveObject;

static PyTypeObject save_SaveType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        "save.Save",             /* tp_name */
    sizeof(save_SaveObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Save objects",           /* tp_doc */
};

static PyMethodDef save_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initsave(void) 
{
    PyObject* m;

    save_SaveType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&save_SaveType) < 0)
        return;

    m = Py_InitModule3("save", save_methods,
            "Example module that creates an extension type.");

    Py_INCREF(&save_SaveType);
    PyModule_AddObject(m, "Save", (PyObject *)&save_SaveType);
}

#include "pydirty.h"

//直接传入元组参数
int call_script_function(const char *module, const char *function, PyObject *args)
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pResult;

    pName = PyUnicode_FromString(module);
    if (pName == NULL) {
        PyErr_Print();
        return 1;
    }

    pModule = PyImport_Import(pName);
    if (pModule == NULL) {
        fprintf(stderr, "Failed to load \"%s\"\n", module);
        PyErr_Print();
        goto import_error;
    }

    pFunc = PyObject_GetAttrString(pModule, function);
    if (pFunc == NULL) {
        fprintf(stderr, "Cannot find function \"%s\"\n", function);
        PyErr_Print();
        goto function_error;
    }

    if (!PyCallable_Check(pFunc)) {
        fprintf(stderr, "Function must be callable\"%s\"\n", function);
        goto call_error;
    }

    if (args != NULL) {
        if (!PyTuple_CheckExact(args)) {
            fprintf(stderr, "args must be tuple\n");
            PyObject_Print(args, stdout, 0);
            goto call_error;
        }
    }

    pResult = PyObject_CallObject(pFunc, args);
    if (pResult == NULL) {
        fprintf(stderr,"Call failed\n");
        PyErr_Print();
        goto call_error;
    }

    Py_DECREF(pName);
    Py_DECREF(pResult);
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    return 0;

call_error:
    Py_DECREF(pFunc);
function_error:
    Py_DECREF(pModule);
import_error:
    Py_DECREF(pName);
    return 1;
}

void init_python_vm()
{
    init_dirty_module();
    Py_Initialize();
    PyRun_SimpleString("import sys\nsys.path.append(\"./script\")");
}

void destroy_python_vm()
{
    Py_Finalize();
}

#include <Python.h>

int call_script_function(const char *module, const char *function, const char *buf, int len)
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pResult;

    pName = PyString_FromString(module);

    pModule = PyImport_Import(pName);
    if (pModule == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", module);
        return 1;
    }

    pFunc = PyObject_GetAttrString(pModule, function);
    if (pFunc == NULL) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Cannot find function \"%s\"\n", function);
        return 1;
    }

    if (!PyCallable_Check(pFunc)) {
        fprintf(stderr, "Function must be callable\"%s\"\n", function);
        return 1;
    }

    pArgs = Py_BuildValue("(s)", buf);
    if (pArgs == NULL) {
        fprintf(stderr, "Build value fail\"%s\"\n", function);
        return 1;
    }
    pResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pResult == NULL) {
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr,"Call failed\n");
        return 1;
    }

    printf("Result of call: %ld\n", PyLong_AsLong(pResult));

    Py_DECREF(pResult);
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    return 0;
}

void init_python_vm()
{
    Py_Initialize();
    PyRun_SimpleString("import sys\nsys.path.append(\"./module\")");
}

void destroy_python_vm()
{
    Py_Finalize();
}

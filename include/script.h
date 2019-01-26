#ifndef __SCRIPT__
#define __SCRIPT__

#include <Python.h>

void init_python_vm();
void destroy_python_vm();
int call_script_function(const char *module, const char *function, const char *buf, int len);
int call_script_func(const char *module, const char *function, PyObject *args);

#endif //__SCRIPT_H__

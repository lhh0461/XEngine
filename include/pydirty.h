#ifndef __PYDIRTY__
#define __PYDIRTY__

#include <Python.h>

struct dirty_mng_s;

typedef struct {
    PyDictObject dict;
    struct dirty_mng_s *dirty_mng;
    PyDictObject *subdocs;
} PyDirtyDictObject;

typedef struct {
    PyListObject list;
    struct dirty_mng_s *dirty_mng;
} PyDirtyListObject;

extern PyTypeObject PyDirtyDict_Type;
extern PyTypeObject PyDirtyList_Type;

#define PyDirtyDict_CheckExact(op) (Py_TYPE(op) == &PyDirtyDict_Type)
#define PyDirtyList_CheckExact(op) (Py_TYPE(op) == &PyDirtyList_Type)

PyDirtyListObject *PyDirtyList_New(Py_ssize_t size);
PyDirtyDictObject *PyDirtyDict_New();

void init_dirty_dict(void);
void init_dirty_list(void);

PyDirtyDictObject *build_dirty_dict(PyDictObject *dict);
PyDirtyListObject *build_dirty_list(PyListObject *list);

#define SUPPORT_DIRTY_VALUE_TYPE(ob) \
    (PyDict_CheckExact(ob) || \
     PyList_CheckExact(ob) || \
     PyLong_CheckExact(ob) || \
     PyFloat_CheckExact(ob) || \
     PyBytes_CheckExact(ob))

int init_dirty_module();

#endif //__PYDIRTY__

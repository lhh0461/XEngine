#ifndef __PYDIRTY__
#define __PYDIRTY__

#include <Python.h>

struct dirty_mng_s;

typedef struct {
    PyDictObject dict;
    struct dirty_mng_s *mng;
} PyDirtyDictObject;

typedef struct {
    PyListObject list;
    struct dirty_mng_s *mng;
} PyDirtyListObject;

extern PyTypeObject PyDirtyDict_Type;

#define PyDirtyDict_CheckExact(op) (Py_TYPE(op) == &PyDirtyDict_Type)

void init_dirty_dict(void);

#endif //__PYDIRTY__

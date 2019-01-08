#ifndef __PYDIRTY__
#define __PYDIRTY__

#include <Python.h>

struct dirty_mng_s;

typedef struct {
    PyDictObject dict;
    struct dirty_mng_s *dirty_mng;
} PyDirtyDictObject;

typedef struct {
    PyListObject list;
    struct dirty_mng_s *dirty_mng;
} PyDirtyListObject;

extern PyTypeObject PyDirtyDict_Type;
extern PyTypeObject PyDirtyList_Type;

#define PyDirtyDict_CheckExact(op) (Py_TYPE(op) == &PyDirtyDict_Type)
#define PyDirtyList_CheckExact(op) (Py_TYPE(op) == &PyDirtyList_Type)

void init_dirty_dict(void);
void init_dirty_list(void);

#endif //__PYDIRTY__

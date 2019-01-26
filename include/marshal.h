#ifndef __PYDIRTY_MARSHAL_H__
#define __PYDIRTY_MARSHAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

#include <stdlib.h>

/*
 ** INTEGER = char|short|int 
 ** STRING = char*
 ** TYPE = INTEGER|STRING|nil
 ** FIELD = INTEGER:integer_value|STRING:string_len:string_data|nil
 ** ATOM = FIELD
 ** KEY = key_len:ATOM+
 ** VALUE = FIELD
 ** GDI = (KEY:VALUE)*
 */

// 保留REAL和BUFFER兼容lpc
#define FS_DBI_INT 0
#define FS_DBI_REAL 1 // lpc: flost
#define FS_DBI_STRING 2
#define FS_DBI_ARRAY 3
#define FS_DBI_MAPPING 4
#define FS_DBI_NIL 5
#define FS_DBI_BUFFER 6
#define FS_DBI_LONG 7
#define FS_DBI_DOUBLE 8

//#define COMMON_FIELD  int type
#define COMMON_FIELD  short type

typedef unsigned FS_DBI_CNT_TYPE;


typedef struct marshal_nil_s
{
    COMMON_FIELD;
} marshal_nil_t;

typedef struct marshal_int_s
{
    COMMON_FIELD;
    int n;
} marshal_int_t;

typedef struct marshal_long_s
{
    COMMON_FIELD;
    long n;
} marshal_long_t;

typedef struct marshal_double_s
{
    COMMON_FIELD;
    double n;
} marshal_double_t;

typedef struct marshal_real_s
{
    COMMON_FIELD;
    float r;
} marshal_real_t;

typedef struct marshal_string_s
{
    COMMON_FIELD;
    unsigned len;
    char str[0];
} marshal_string_t;

typedef marshal_string_t marshal_buffer_t;

typedef struct marshal_arr_s
{
    COMMON_FIELD;
    int cnt;
} marshal_arr_t;

typedef struct marshal_mapping_s
{
    COMMON_FIELD;
    int pair;
} marshal_mapping_t;


typedef union marshal_tvalue_u 
{
    struct {
        COMMON_FIELD;
    } common;
    marshal_nil_t nil;
    marshal_int_t number;
    marshal_long_t nlong;
    marshal_double_t ndouble;
    marshal_real_t real;
    marshal_string_t string;
    marshal_arr_t arr;
    marshal_mapping_t map;
} marshal_tvalue_t;

////////////////////////////////

typedef struct marshal_array_s
{
    unsigned *cnt;
    buffer_t buf;
} marshal_array_t;


void marshal_array_construct(marshal_array_t *array);
void marshal_array_destruct(marshal_array_t *array);
void marshal_array_reset(marshal_array_t *array);
int marshal_array_datalen(marshal_array_t *array);

void *marshal_array_alloc(marshal_array_t *array, unsigned size);

void marshal_array_push_int(marshal_array_t *array, int n);
void marshal_array_push_long(marshal_array_t *array, long n);
void marshal_array_push_double(marshal_array_t *array, double n);
//void marshal_array_push_real(marshal_array_t *array, float r);
void marshal_array_push_nil(marshal_array_t *array);

void marshal_array_push_string(marshal_array_t *array,  const char *str);
void marshal_array_push_lstring(marshal_array_t *array,  const char *str, size_t len);

//void marshal_array_push_buffer(marshal_array_t *array,  unsigned char *buf, size_t len);

void marshal_array_push_array(marshal_array_t *array, int cnt);
void marshal_array_set_array(marshal_array_t *array, void *arr, int cnt);

void marshal_array_push_mapping(marshal_array_t *array, int cnt);
void marshal_array_set_mapping(marshal_array_t *array, void *map, int pair);

int marshal_tvalue_size(marshal_tvalue_t *tvalue);


typedef struct marshal_array_iter_s {
    unsigned i;
    FS_DBI_CNT_TYPE cnt;
    void *arr;
    unsigned size;
    marshal_tvalue_t *tv;
} marshal_array_iter_t;

inline static void FS_DBI_ARRAY_ITER_FIRST(marshal_array_iter_t *iter, void *arr, unsigned size) 
{
    FS_DBI_CNT_TYPE *cnt = (FS_DBI_CNT_TYPE *)arr;
    iter->i = 0;
    iter->cnt = *cnt;
    iter->arr = arr;
    iter->size = size;
    iter->tv = iter->cnt == 0 ? NULL : (marshal_tvalue_t*)(cnt + 1);
}

inline static void FS_DBI_ARRAY_ITER_NEXT(marshal_array_iter_t *iter)
{
    iter->tv = ++iter->i < iter->cnt ? 
        (marshal_tvalue_t*)((char *)iter->tv + marshal_tvalue_size(iter->tv))
        : NULL;
}

inline static unsigned FS_DBI_ARRAY_LEN(marshal_array_t *array) 
{
    unsigned size = array->buf.data_size;

    if (size >= sizeof(FS_DBI_CNT_TYPE)) {
        return size - sizeof(FS_DBI_CNT_TYPE);
    } else {
        return 0;
    }
}

inline static unsigned FS_DBI_ITER_FIELD_TYPE(marshal_array_iter_t *iter) 
{
    return iter->tv->common.type;
}

int marshal(PyObject *value, marshal_array_t *arr);
PyObject *unmarshal(marshal_array_iter_t *iter);

void marshal_test();
#ifdef __cplusplus
}
#endif

#endif /*__PYDIRTY_MARSHAL_H__*/


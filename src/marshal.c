#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pydirty.h"
#include "marshal.h"
#include "util.h"


void marshal_array_construct(marshal_array_t *array)
{
    memset(array, 0, sizeof(marshal_array_t));
    buffer_init(&array->buf);
    array->cnt = (unsigned int *)buffer_malloc(&array->buf, sizeof(*array->cnt));
    *array->cnt = 0;
}

void marshal_array_reset(marshal_array_t *array)
{
    buffer_reset(&array->buf);
    array->cnt = (unsigned int *)buffer_malloc(&array->buf, sizeof(*array->cnt));
    *array->cnt = 0;
}

void marshal_array_destruct(marshal_array_t *array)
{
    array->cnt = NULL;
    buffer_release(&array->buf);
}

int marshal_array_datalen(marshal_array_t *array)
{
    return array->buf.data_size;
}

void *marshal_array_alloc(marshal_array_t *array, unsigned size)
{
    void *val = buffer_malloc(&array->buf, size);
    (*array->cnt)++;
    return val;
}

void marshal_array_push_int(marshal_array_t *array, int n)
{
    //printf("%s\n", __func__);
    marshal_int_t *val = (marshal_int_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_INT;
    val->n = n;
}

void marshal_array_push_long(marshal_array_t *array, long n)
{
    marshal_long_t *val = (marshal_long_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_LONG;
    val->n = n;
}

void marshal_array_push_double(marshal_array_t *array, double n)
{
    marshal_double_t *val = (marshal_double_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_DOUBLE;
    val->n = n;
}

void marshal_array_push_nil(marshal_array_t *array)
{
    //printf("%s\n", __func__);
    marshal_nil_t *val = (marshal_nil_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_NIL;
}

void marshal_array_push_string(marshal_array_t *array, const char *str)
{
    //printf("%s\n", __func__);
    //the caller check the str not to be null
    size_t len = strlen(str);
    marshal_string_t *val = (marshal_string_t *)marshal_array_alloc(array, sizeof(*val) + len + 1);
    val->type = FS_DBI_STRING;
    val->len = len + 1;
    memcpy(val->str, str, len);
    *((char *)val->str + len) = '\0';
}

void marshal_array_push_buffer(marshal_array_t *array, const char *buf, size_t len)
{
    //printf("%s\n", __func__);
    marshal_buffer_t *val = (marshal_buffer_t *)marshal_array_alloc(array, sizeof(*val) + len);
    val->type = FS_DBI_BUFFER;
    val->len = len;
    memcpy(val->str, buf, len);
}

void marshal_array_push_array(marshal_array_t *array, int cnt)
{
    //printf("%s\n", __func__);
    marshal_arr_t *val = (marshal_arr_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_ARRAY;
    val->cnt = cnt;
}

void marshal_array_set_array(marshal_array_t *array, void *arr, int cnt)
{
    //printf("%s\n", __func__);
    marshal_arr_t *val = (marshal_arr_t *)arr;
    val->type = FS_DBI_ARRAY;
    val->cnt = cnt;
}

void marshal_array_push_mapping(marshal_array_t *array, int pair)
{
    //printf("%s\n", __func__);
    marshal_mapping_t *val = (marshal_mapping_t *)marshal_array_alloc(array, sizeof(*val));
    val->type = FS_DBI_MAPPING;
    val->pair = pair;
}

void marshal_array_set_mapping(marshal_array_t *array, void *map, int pair)
{
    //printf("%s\n", __func__);
    marshal_mapping_t *val = (marshal_mapping_t *)map;
    val->type = FS_DBI_MAPPING;
    val->pair = pair;
}

int marshal_tvalue_size(marshal_tvalue_t *tvalue)
{
    switch (tvalue->common.type) {
        case FS_DBI_NIL:	
            return sizeof(marshal_nil_t);
        case FS_DBI_INT:
            return sizeof(marshal_int_t);
        case FS_DBI_LONG:
            return sizeof(marshal_long_t);
        case FS_DBI_DOUBLE:
            return sizeof(marshal_double_t);
        case FS_DBI_REAL:
            return sizeof(marshal_real_t);
        case FS_DBI_ARRAY:	
            return sizeof(marshal_arr_t);
        case FS_DBI_MAPPING:
            return sizeof(marshal_mapping_t);
        case FS_DBI_STRING:
            return (sizeof(marshal_string_t) + tvalue->string.len);
        case FS_DBI_BUFFER:
            return (sizeof(marshal_buffer_t) + tvalue->string.len);
        default:
            fprintf(stderr, "[unmarshal] invalid type:%d\n", tvalue->common.type);
            print_trace();
            return -1;
    }
}

static int marshal_array(PyObject *array, marshal_array_t *arr);
static int marshal_mapping(PyObject *map, marshal_array_t *arr);

int marshal(PyObject *value, marshal_array_t *arr)
{
    if (value == NULL) {
        marshal_array_push_nil(arr);
    }  else if (PyLong_CheckExact(value)) {
        long tmp = PyLong_AsLong(value);
        if (tmp == -1 && PyErr_ExceptionMatches(PyExc_OverflowError)) {
            return -1;
        }
        //printf("marshal long = %ld\n", tmp);
        marshal_array_push_long(arr, tmp);
    } else if (PyFloat_CheckExact(value)) {
        marshal_array_push_double(arr, PyFloat_AS_DOUBLE(value));
    } else if (PyUnicode_CheckExact(value)) {
        //printf("marshal str = %s\n", PyUnicode_AsUTF8AndSize(value, NULL));
        marshal_array_push_string(arr, PyUnicode_AsUTF8(value));
    } else if (PyDict_CheckExact(value) || PyDirtyDict_CheckExact(value)) {
        if (marshal_mapping(value, arr)) return -1;
    } else if (PyList_CheckExact(value) || PyDirtyList_CheckExact(value)) {
        if (marshal_array(value, arr)) return -1;
    } else {
        fprintf(stderr, "[marshal] unknow data type [%s]\n", Py_TYPE(value)->tp_name);
        print_trace();
        return -1;
    }

    return 0;
}

static int marshal_mapping(PyObject *map, marshal_array_t *arr)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    marshal_array_push_mapping(arr, PyDict_Size(map));
    //printf("marshal mapping map=%p\n", map);

    while (PyDict_Next(map, &pos, &key, &value)) {
        //printf("marshal mapping key value\n");
        if (marshal(key, arr)) return -1;
        if (marshal(value, arr)) return -1;
    }

    return 0;
}

int marshal_array(PyObject *array, marshal_array_t *arr)
{
    Py_ssize_t i, size;
    PyObject *item;

    size = PyList_GET_SIZE(array);
    marshal_array_push_array(arr, size);

    for(i = 0; i < size; ++i) {
        item = PyList_GET_ITEM(array, i);
        if (marshal(item, arr)) return -1;
    }

    return 0;
}

static PyObject *unmarshal_array(marshal_array_iter_t *iter)
{
    int i;
    int cnt = iter->tv->arr.cnt;
    PyObject *item;
    PyObject *arr = PyList_New(cnt);

    FS_DBI_ARRAY_ITER_NEXT(iter); //skip the array header
    for (i = 0; i < cnt; i++) {
        item = unmarshal(iter);
        if (item == NULL) {
            Py_DECREF(arr);
            return NULL;
        }

        PyList_SET_ITEM(arr, i, item);
    }

    return arr;
}


PyObject *unmarshal_mapping(marshal_array_iter_t *iter)
{
    int i;
    PyObject *k, *v;
    int pair = iter->tv->map.pair;
    PyObject *map = PyDict_New();

    FS_DBI_ARRAY_ITER_NEXT(iter); //skip the map header
    for(i = 0; i < pair; ++i) {
        if ((k = unmarshal(iter)) == NULL) { //must be string or number
            Py_DECREF(map);
            return NULL;
        }

        if (!PyLong_CheckExact(k) && !PyUnicode_CheckExact(k)) {
            fprintf(stderr, "[unmarshal] fail, not support type:%s\n", Py_TYPE(k)->tp_name);
            Py_DECREF(k);
            Py_DECREF(map);
            return NULL;
        }

        if ((v = unmarshal(iter)) == NULL) {
            Py_DECREF(k);
            Py_DECREF(map);
            return NULL;
        }

        PyDict_SetItem(map, k, v);
        Py_DECREF(k);
        Py_DECREF(v);
    }

    return map;
}

PyObject *unmarshal(marshal_array_iter_t *iter)
{
    PyObject *v;
    marshal_tvalue_t *tv = iter->tv;
    //printf("on unmarshal start\n");

    switch (tv->common.type) {
        case FS_DBI_NIL:
            Py_INCREF(Py_None);
            v = Py_None;
            FS_DBI_ARRAY_ITER_NEXT(iter);
            break;
        case FS_DBI_LONG:
            v = PyLong_FromLong(tv->nlong.n);
            FS_DBI_ARRAY_ITER_NEXT(iter);
            //printf("on unmarshal int =%ld\n", tv->nlong.n);
            break;
        case FS_DBI_REAL: // real: lpc float
            v = PyFloat_FromDouble(tv->real.r);
            FS_DBI_ARRAY_ITER_NEXT(iter);
            //printf("on unmarshal real\n");
            break;
        case FS_DBI_DOUBLE:
            v = PyFloat_FromDouble(tv->ndouble.n);
            FS_DBI_ARRAY_ITER_NEXT(iter);
            //printf("on unmarshal double\n");
            break;
        case FS_DBI_BUFFER:
            v = PyBytes_FromStringAndSize(tv->string.str, tv->string.len);
            FS_DBI_ARRAY_ITER_NEXT(iter);
            //printf("on unmarshal buffer\n");
            break;
        case FS_DBI_STRING:
            v = PyUnicode_FromString(tv->string.str);
            FS_DBI_ARRAY_ITER_NEXT(iter);
            //printf("on unmarshal string\n");
            break;
        case FS_DBI_ARRAY:
            v = unmarshal_array(iter);
            //printf("on unmarshal array\n");
            break;
        case FS_DBI_MAPPING:
            v = unmarshal_mapping(iter);
            //printf("on unmarshal mapping\n");
            break;
        default:
            fprintf(stderr, "[unmarshal] unknow data type [%d]\n", tv->common.type);
            print_trace();
            return NULL;
    }

    return v;
}

#include "bson_format.h"

void marshal_test()
{
   bson_t b;
   bson_t child;  
   PyObject *ob = PyDict_New();
   PyObject *out;
   marshal_array_iter_t iter;
   marshal_array_t arr;
   marshal_array_construct(&arr);

   bson_init (&b);
   BSON_APPEND_INT32 (&b, "123", 1);
   BSON_APPEND_INT32 (&b, "hello", 2);

   BSON_APPEND_DOCUMENT_BEGIN(&b,"doc", &child); 
   BSON_APPEND_INT32 (&child, "11", 1);
   BSON_APPEND_INT64 (&child, "uid", 1);
   bson_append_document_end(&b, &child); 

   BSON_APPEND_ARRAY_BEGIN(&b, "arr", &child); 
   BSON_APPEND_DOUBLE(&child, "0", 1.0);
   BSON_APPEND_DOUBLE(&child, "1", 1.3333);
   BSON_APPEND_INT64 (&child, "2", 4);
   bson_append_array_end(&b, &child); 

   bson_to_pydict(&b, ob);

   assert(marshal(ob, &arr) == 0);
   PyObject_Print(ob, stdout, 0);
   printf("print ob\n");

   char *bytes = buffer_pullup(&arr.buf);
   unsigned len = arr.buf.data_size;

   FS_DBI_ARRAY_ITER_FIRST(&iter, bytes, len);
   out = unmarshal(&iter);

   PyObject_Print(ob, stdout, 0);
   printf("print ob\n");
   PyObject_Print(out, stdout, 0);
   printf("print out\n");

/*
   marshal_array_iter_t iter;
   marshal_array_t arr;
   marshal_array_construct(&arr);
   marshal_array_push_mapping(&arr, 2);
   marshal_array_push_string(&arr, "key1");
   marshal_array_push_int(&arr, 100);
   marshal_array_push_string(&arr, "key2");
   marshal_array_push_int(&arr, 1000);

   char *bytes = buffer_pullup(&arr.buf);
   unsigned len = arr.buf.data_size;
   FS_DBI_ARRAY_ITER_FIRST(&iter, bytes, len);
   printf("on unmarshal mapsize =%d\n", iter.tv->map.pair);
   assert(FS_DBI_ITER_FIELD_TYPE(&iter) == FS_DBI_MAPPING);
   FS_DBI_ARRAY_ITER_NEXT(&iter); //skip the map header
   printf("on unmarshal string =%s\n", iter.tv->string.str);
   assert(FS_DBI_ITER_FIELD_TYPE(&iter) == FS_DBI_STRING);
   FS_DBI_ARRAY_ITER_NEXT(&iter); //skip the map header
   assert(FS_DBI_ITER_FIELD_TYPE(&iter)  == FS_DBI_INT);
   printf("on unmarshal int =%d\n", iter.tv->number.n);
   FS_DBI_ARRAY_ITER_NEXT(&iter); //skip the map header
   printf("on unmarshal string =%s\n", iter.tv->string.str);
   assert(FS_DBI_ITER_FIELD_TYPE(&iter) == FS_DBI_STRING);
   FS_DBI_ARRAY_ITER_NEXT(&iter); //skip the map header
   assert(FS_DBI_ITER_FIELD_TYPE(&iter) ==  FS_DBI_INT);
   printf("on unmarshal int =%d\n", iter.tv->number.n);
   */
}

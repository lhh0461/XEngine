#include "bson_format.h"
#include "log.h"
#include "util.h"

static void _bson_to_pydict(const bson_t *b, PyObject *out);
static void _bson_to_pylist(bson_t *b , PyObject *out);
static void _pydict_to_bson(PyObject * dict, bson_t * b);
static void _pylist_to_bson(PyObject *list, bson_t * b);

static const char *bson_type_str(bson_type_t type) 
{
    switch(type)
    {
        case BSON_TYPE_EOD:
            return "BSON_TYPE_EOD "; 
        case BSON_TYPE_DOUBLE: 
            return "BSON_TYPE_DOUBLE"; 
        case BSON_TYPE_UTF8:
            return "BSON_TYPE_UTF8";
        case BSON_TYPE_DOCUMENT:
            return "BSON_TYPE_DOCUMENT" ; 
        case BSON_TYPE_ARRAY:
            return "BSON_TYPE_ARRAY";  
        case BSON_TYPE_BINARY:
            return "BSON_TYPE_BINARY"; 
        case BSON_TYPE_UNDEFINED:
            return "BSON_TYPE_UNDEFINED"; 
        case BSON_TYPE_OID:
            return "BSON_TYPE_OID"; 
        case BSON_TYPE_BOOL:
            return "BSON_TYPE_BOOL"; 
        case BSON_TYPE_DATE_TIME:
            return "BSON_TYPE_DATE_TIME"; 
        case BSON_TYPE_NULL:
            return "BSON_TYPE_NULL"; 
        case BSON_TYPE_REGEX:
            return "BSON_TYPE_REGEX"; 
        case BSON_TYPE_DBPOINTER:
            return "BSON_TYPE_DBPOINTER"; 
        case BSON_TYPE_CODE:
            return "BSON_TYPE_CODE"; 
        case BSON_TYPE_SYMBOL:
            return "BSON_TYPE_SYMBOL"; 
        case BSON_TYPE_CODEWSCOPE:
            return "BSON_TYPE_CODEWSCOPE"; 
        case BSON_TYPE_INT32:
            return "BSON_TYPE_INT32"; 
        case BSON_TYPE_TIMESTAMP:
            return "BSON_TYPE_TIMESTAMP"; 
        case BSON_TYPE_INT64:
            return "BSON_TYPE_INT64"; 
        case BSON_TYPE_MAXKEY:
            return "BSON_TYPE_MAXKEY"; 
        case BSON_TYPE_MINKEY:
            return "BSON_TYPE_MINKEY"; 
        default:
            return "unknown type";
    }
}

static void _pylist_to_bson(PyObject *list, bson_t * b)
{
    PyObject * value;
    Py_ssize_t pos = 0;
    Py_ssize_t size = PyList_GET_SIZE(list);
    bson_t child;  

    for (; pos < size; pos++)
    {
        value = PyList_GET_ITEM((PyObject *)list, pos);
        if (PyUnicode_CheckExact(value)) {
            BSON_APPEND_UTF8(b, "", PyUnicode_AsUTF8AndSize(value, NULL));
        }
        else if (PyLong_CheckExact(value)) {
            BSON_APPEND_INT32(b, "", PyLong_AS_LONG(value));
        }
        else if (PyDict_CheckExact(value)) {
            BSON_APPEND_DOCUMENT_BEGIN(b, "", &child); 
            _pydict_to_bson(value, &child); 
            bson_append_document_end(b, &child); 
        }
        else if (PyList_CheckExact(value)) {
            BSON_APPEND_ARRAY_BEGIN(b, "", &child); 
            _pylist_to_bson(value,&child);
            bson_append_array_end(b, &child); 
        }
        else if (PyFloat_CheckExact(value)) {
            BSON_APPEND_DOUBLE(b, "", PyFloat_AS_DOUBLE(value));
        } 
        else {
            log_error("_pylist_to_bson unsupported type:%s.",Py_TYPE(value)->tp_name); 
            INTERNAL_ASSERT(0);
        }
    } 
}

static void _pydict_to_bson(PyObject * dict, bson_t * b)
{
    PyObject *k, *v; 
    bson_t child;  
    long num; 
    double real; 
    char * key , buf[MAX_KEY_LEN]; 
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &k, &v)) {
        memset(buf, 0 ,sizeof(buf)); 			
        if (PyLong_CheckExact(k)) {
            num = PyLong_AS_LONG(k); 
            sprintf(buf, "__i__%ld", num); 
            key = buf; 
        }
        else if (PyUnicode_CheckExact(k)) {
            PyObject *bytes = PyUnicode_AsUTF8String(k); 
            char *str = PyBytes_AsString(bytes);
            memcpy(buf, str, strlen(str));
            key = buf; 
        }
        else {
            log_error("mapping_to_bson unsupported type:%s.",Py_TYPE(k)->tp_name); 
            assert(0);
        }

        INTERNAL_ASSERT(key && strlen(key));
        if(!bson_utf8_validate(key,strlen(key),false))
        {
            log_error("not utf8:%s.", key); 	
            INTERNAL_ASSERT(0);
        }

        if (!key || !strlen(key))
        {
            log_error("warn: key error. key is null or zero string");
            continue;
        }

        if (PyUnicode_CheckExact(v)) {
            PyObject *bytes = PyUnicode_AsUTF8String(v); 
            printf("abc=%s\n", PyBytes_AsString(bytes));
            printf("key=%s\n", key);
            BSON_APPEND_UTF8(b, key, PyBytes_AsString(bytes));
        }
        else if (PyLong_CheckExact(v)) {
            long abc = PyLong_AS_LONG(v);
            printf("key=%s\n", key);
            printf("value=%ld\n", abc);
            BSON_APPEND_INT32(b, key, abc);
            //BSON_APPEND_INT64(b, key, PyLong_AS_LONG(v));
        }
        else if (PyDict_CheckExact(v)) {
            BSON_APPEND_DOCUMENT_BEGIN(b,key, &child); 
            _pydict_to_bson(v, &child); 
            bson_append_document_end(b, &child); 
        }
        else if (PyList_CheckExact(v)) {
            BSON_APPEND_ARRAY_BEGIN(b, key, &child); 
            _pylist_to_bson(v,&child);
            bson_append_array_end(b, &child); 
        }
        else if (PyFloat_CheckExact(v)) {
            BSON_APPEND_DOUBLE(b, key, PyFloat_AS_DOUBLE(v));
        } 
        else if (PyBool_Check(v)) {
            BSON_APPEND_BOOL(b, key, PyLong_AS_LONG(v));
        }
        else {
            log_error("_mapping_to_bson unsupported type:%s", Py_TYPE(v)->tp_name); 
            INTERNAL_ASSERT(0);
        }
    }
}

int pylist_to_bson(PyObject *list, bson_t * out)
{
#ifdef DEVELOP_MODE
    size_t 	offset; 
    bool 	validate; 
#endif

    if (!list) {
        return 0;
    }

    if (!PyList_CheckExact(list)) {
        return 0;
    }

    _pylist_to_bson(list, out); 

#ifdef 	DEVELOP_MODE
    validate = bson_validate(out, BSON_VALIDATE_UTF8,&offset); 
    // offset 值并不能友好地指出出错数据
    if(!validate)
    {
        assert(false); 
    }
    /*
       BSON_VALIDATE_UTF8 = (1 << 0),
       BSON_VALIDATE_DOLLAR_KEYS = (1 << 1),
       BSON_VALIDATE_DOT_KEYS = (1 << 2),
       BSON_VALIDATE_UTF8_ALLOW_NULL = (1 << 3),
       */
#endif

    return 1; 
}

int pydict_to_bson(PyObject *dict, bson_t * out)
{
#ifdef DEVELOP_MODE
    size_t 	offset; 
    bool 	validate; 
#endif

    if (!dict) {
        return 0;
    }

    if (!PyDict_CheckExact(dict)) {
        return 0;
    }

    _pydict_to_bson(dict, out); 

#ifdef 	DEVELOP_MODE
    validate = bson_validate(out, BSON_VALIDATE_UTF8,&offset); 
    // offset 值并不能友好地指出出错数据
    if(!validate)
    {
        assert(false); 
    }
    /*
       BSON_VALIDATE_UTF8 = (1 << 0),
       BSON_VALIDATE_DOLLAR_KEYS = (1 << 1),
       BSON_VALIDATE_DOT_KEYS = (1 << 2),
       BSON_VALIDATE_UTF8_ALLOW_NULL = (1 << 3),
       */
#endif

    return 1; 
}

/*
// not thread safe !
void check_lpc_key(svalue_t *in , char *out)
{
    int num , len; 
    float real; 
    char * key; 

    static char buf[MAX_KEY_LEN] ; 
    //memset(buf, 0, MAX_KEY_LEN); 

    switch( in->type ) 
    {
        case T_NUMBER:
            num = LPC_INT_VALUE(in); 
            //sprintf(buf, I_FORMAT, num); 
            I_LPC_T_BSON(buf, MAX_KEY_LEN, num)
                key = buf; 
            break; 

        case T_REAL:
            real = LPC_REA_VALUE(in); 
            F_LPC_T_BSON(buf, MAX_KEY_LEN, real)
                //sprintf(buf, F_FORMAT, real); 
                key = buf; 
            break; 

        case T_STRING:
            key = LPC_STR_VALUE(in); 	
            len = strlen(key);
            if(len >=MAX_KEY_LEN) {
                printf("lpc key too large:<<%s>>\n", key); 
                fflush(NULL); 
#ifdef DEVELOP_MODE
                assert(NULL); 
#endif
            }

            if(strchr(key, '.')) {
                printf("invalid string key:<<%s>> containing dot\n", key);
                fflush(NULL); 
#ifdef DEVELOP_MODE
                assert(NULL); 
#endif
            }
            if(len > 0 && key[0] == '$'){
                printf("invalid string key: <<%s>> ,cannot start with $ \n", key);
                fflush(stdout); 
#ifdef DEVELOP_MODE
                assert(NULL); 
#endif
            }

            break; 

        default:
            assert(NULL); 
    }

    strcat(out, "."); 
    strcat(out, key); 
}
*/


char int_key_prefix[] = "__i__";

static PyObject * check_bson_key(const char * strkey)
{
    assert(strlen(strkey)); 

    if (*strkey != '_') {
        return PyUnicode_FromString(strkey);
    }

    if (strncmp(strkey, int_key_prefix, strlen(int_key_prefix)) == 0) {
        int num;
        sscanf( strkey, I_FORMAT, &num );
        return PyLong_FromLong(num);
    }
    else {
        return PyUnicode_FromString(strkey);
    }
}

static void _bson_to_pylist(bson_t *b , PyObject *out)
{
    bson_t sub_b; 
    bson_iter_t iter; 
    uint32_t len, arraylen, num32; 
    uint64_t num64; 
    const uint8_t *buff; 
    double flot; 
    bson_type_t type; 
    PyObject * map; 
    char *str; 
    PyObject * value; 
    Py_ssize_t pos = 0;

    arraylen = bson_count_keys(b); 

    bson_iter_init(&iter, b); 
    while(bson_iter_next(&iter)) 
    {
        type = bson_iter_type(&iter); 
        switch( type ) 
        {
            case BSON_TYPE_INT32:
                PyList_SET_ITEM(out, pos++, PyLong_FromLong(bson_iter_int32(&iter)));
                continue; 

            case BSON_TYPE_INT64:
                PyList_SET_ITEM(out, pos++, PyLong_FromLong(bson_iter_int64(&iter)));
                continue; 

            case BSON_TYPE_DOUBLE:
                PyList_SET_ITEM(out, pos++, PyFloat_FromDouble(bson_iter_double(&iter)));
                continue; 

            case BSON_TYPE_UTF8:
                PyList_SET_ITEM(out, pos++, PyUnicode_FromString(bson_iter_utf8(&iter, NULL)));
                continue; 

            case BSON_TYPE_DOCUMENT:
                bson_iter_document(&iter,&len, &buff); 
                bson_init_static(&sub_b,buff,len); 

                value = PyDict_New(); 	
                _bson_to_pydict(&sub_b, value); 
                PyList_SET_ITEM(out, pos++, value);
                continue; 

            case BSON_TYPE_ARRAY:
                bson_iter_array(&iter,&len, &buff); 
                bson_init_static(&sub_b,buff,len); 

                value = PyList_New(bson_count_keys(&sub_b)); 	
                _bson_to_pylist(&sub_b, value); 
                PyList_SET_ITEM(out, pos++, value);
                continue; 

            default:
                log_info("_bson_to_array unsupport type:%s." , bson_type_str(type)); 
#ifdef DEVELOP_MODE
                assert( 0 ); 
#endif
        }
    }
}

static void _bson_to_pydict(const bson_t *b, PyObject *out)
{
    bson_iter_t iter; 
    bson_t sub_b; 
    bson_type_t type; 
    const char *strkey; 
    char  *str; 
    PyObject *key, *value; 
    PyObject * dict;
    uint32_t len; 
    const uint8_t * buff; 

    bson_iter_init(&iter, b); 

    while(bson_iter_next(&iter)) 
    {
        type = bson_iter_type(&iter); 
        strkey = bson_iter_key(&iter); 
        key = check_bson_key(strkey); 
        if (key == NULL) {
            fprintf(stderr, "invalid bson key: %s\n", strkey);
            INTERNAL_ASSERT(0); 
        }

        switch(type) 
        {
            case BSON_TYPE_OID:
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_NULL:
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_BOOL:
                PyDict_SetItem(out, key, bson_iter_bool(&iter) ? Py_True : Py_False); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_INT32:
                PyDict_SetItem(out, key, PyLong_FromLong(bson_iter_int32(&iter))); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_INT64:
                PyDict_SetItem(out, key, PyLong_FromLong(bson_iter_int64(&iter))); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_DOUBLE:
                PyDict_SetItem(out, key, PyFloat_FromDouble(bson_iter_double(&iter))); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_UTF8:
                PyDict_SetItem(out, key, PyUnicode_FromString(bson_iter_utf8(&iter, NULL))); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_DOCUMENT:
                bson_iter_document(&iter,&len,&buff); 
                bson_init_static(&sub_b,buff,len); 

                value = PyDict_New(); 	
                _bson_to_pydict(&sub_b, value); 

                PyDict_SetItem(out, key, value); 
                Py_DECREF(key);
                continue; 

            case BSON_TYPE_ARRAY:
                bson_iter_array(&iter,&len,&buff); 
                bson_init_static(&sub_b,buff,len); 

                Py_ssize_t len = bson_count_keys(&sub_b); 
                value = PyList_New(len); 	
                _bson_to_pylist(&sub_b, value); 
                PyDict_SetItem(out, key, value); 

                Py_DECREF(key);
                continue; 

            default:
                log_info("_bson_to_mapping unknow type:%s.", bson_type_str(type)); 	
#ifdef DEVELOP_MODE 
                assert( 0 ); 
#endif
                break; 
        }
    }
}


int bson_to_pydict(const bson_t *b, PyObject *out)
{
    assert(PyDict_CheckExact(out));

    bson_iter_t iter; 
    bson_iter_init(&iter, b); 	

    if(!bson_iter_next(&iter)) 
    {
        goto error; 
    }

    if(bson_iter_type(&iter) == BSON_TYPE_EOD) 
    {
#ifdef DEVELOP_MODE 
        assert(false); 
#endif
        goto error;  
    }

    _bson_to_pydict(b,out); 

    return 1; 
error: 
    return 0; 
}

void format_test()
{
   bson_t b;
   bson_t out;
   bson_t child;  
   char *orijson;
   char *outjson;
   PyObject *ob = PyDict_New();

   bson_init (&b);
   bson_init (&out);
   BSON_APPEND_INT32 (&b, "__i__1", 1);
   BSON_APPEND_UTF8 (&b, "hello", "world");
   BSON_APPEND_BOOL (&b, "bool", true);

   BSON_APPEND_DOCUMENT_BEGIN(&b,"doc", &child); 
   BSON_APPEND_INT32 (&child, "11", 1);
   BSON_APPEND_INT64 (&child, "uid", 1);
   BSON_APPEND_UTF8(&child, "234", "1234");
   bson_append_document_end(&b, &child); 

   BSON_APPEND_ARRAY_BEGIN(&b, "arr", &child); 
   BSON_APPEND_DOUBLE(&child, "0", 1.0);
   BSON_APPEND_DOUBLE(&child, "1", 1.3333);
   BSON_APPEND_INT64 (&child, "2", 4);
   BSON_APPEND_UTF8(&child, "2", "1234");
   bson_append_array_end(&b, &child); 

   orijson = bson_as_json (&b, NULL);

   bson_to_pydict(&b, ob);
   PyObject_Print(ob, stdout, 0);
   pydict_to_bson(ob, &out);

   outjson = bson_as_json (&out, NULL);
   printf ("orijson=%s\n", bson_as_json (&b, NULL));
   printf ("outjson=%s\n", outjson);

   bson_free (orijson);
   bson_free (outjson);
   bson_destroy (&b);
   bson_destroy (&out);
}

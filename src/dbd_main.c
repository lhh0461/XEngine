#include <bson.h>
#include <assert.h>
#include <mongoc.h>
#include <stdio.h>
#include <mongoc-client.h>
#include <glib.h>

#include "db.h"
#include "log.h"
#include "marshal.h"
#include "bson_format.h"
#include "protocol_dbd.h"
#include "network.h"
#include "common.h"
#include "start_up.h"

//存储与gamed的连接
connection_t * g_gamed_conn = NULL;
connection_t * g_gamed_conn_sync = NULL;

//连接池
mongoc_client_pool_t * g_client_pool = NULL;

//存储所有存盘对象
GHashTable *g_db_object_table = NULL;

static guint db_object_hash_fun(gconstpointer v)
{
    db_key_t *key = (db_key_t *)v;
    if (key->type == INT_TYPE) return g_int_hash(key->value);
    if (key->type == STRING_TYPE) return g_str_hash(key->value);
}

static gboolean db_object_equal_fun(gconstpointer v1, gconstpointer v2) 
{
    db_key_t *key1 = (db_key_t *)v1;
    db_key_t *key2 = (db_key_t *)v2;
    if (key1->type != key2->type) {
        return 0;
    }
    if (key1->type == INT_TYPE) {
        return *((int *)key1->value) == *((int *)key2->value);
    }
    if (key1->type == STRING_TYPE) {
        return strcmp((char *)key1->value, (char *)key2->value) == 0;
    }
    return 0;
}

void db_key_init(db_key_t *key, int type, int len, void *value)
{
    key->type = type;
    key->len = len; 

    if (type == INT_TYPE) {
        key->value = malloc(key->len);
        memcpy(key->value, value, key->len);
    } else {
        key->value = malloc(key->len + 1);
        memcpy(key->value, value, key->len);
        *((char *)key->value + key->len) = '\0';
        printf("db_key_init value=%s, len=%d, key=%s,keylen=%d,strlen=%ld\n", (char *)value, len, (char *)key->value, key->len, strlen((char *)key->value));
    }
}

db_obj_t *db_object_new(db_key_t *key)
{
    db_obj_t *dbo = (db_obj_t *)calloc(1, sizeof(db_obj_t));
    db_key_init(&dbo->db_key, key->type, key->len, key->value);
    dbo->obj = PyDict_New();
    return dbo;
}

void db_object_free(db_obj_t *obj)
{
    Py_DECREF(obj->obj);
    free(obj->db_key.value);
    free(obj);
}

void init_mongod()
{
    bson_t reply;
    bson_t keys;
    mongoc_index_opt_t opt;
    mongoc_collection_t *collection;
    bson_error_t error;
    mongoc_uri_t *uri;
    mongoc_client_t *client;

    mongoc_init();

    uri = mongoc_uri_new("mongodb://127.0.0.1:27017");
    if (!uri) {
        exit(EXIT_FAILURE);
    }
    g_client_pool = mongoc_client_pool_new(uri);
    assert(g_client_pool);
        
    bson_init(&reply);
    client = mongoc_client_pool_pop(g_client_pool); 

    bool r = mongoc_client_get_server_status(client, (mongoc_read_prefs_t*)NULL, &reply, &error);
    if (!r) {
        printf("mongod server status error. code=%d,message=%s\n", error.code, error.message);
        exit(EXIT_FAILURE);
    }   
    mongoc_uri_destroy(uri);
    bson_destroy(&reply);

    //初始化索引
    bson_init(&keys);
    BSON_APPEND_INT32(&keys,INDEXING,1); 

    mongoc_index_opt_init(&opt);
    opt.unique = true ;

    collection = mongoc_client_get_collection(client,"q1","dat"); 
    if(!mongoc_collection_create_index(collection,&keys,&opt,&error)) {
        LOG_MONGOC_ERROR(error); 
        exit(1); 
    }    
    mongoc_collection_destroy(collection);

    collection = mongoc_client_get_collection(client,"q1","dat"); 
    if(!mongoc_collection_create_index(collection,&keys,&opt,&error)) {
        LOG_MONGOC_ERROR(error); 
        exit(1); 
    } 
    mongoc_collection_destroy(collection);
    mongoc_client_pool_push(g_client_pool, client);
}

void init_query_bson(db_key_t *key, bson_t *query)
{
    if (key->type == STRING_TYPE) {
        printf("query doc is %s\n", (char *)(key->value));
        printf("query key doc is %p\n", key->value);
        BSON_APPEND_UTF8(query, INDEXING, key->value);
    } else if (key->type == INT_TYPE) {
        printf("query doc key is %d\n",  *((int *)key->value));
        BSON_APPEND_INT32(query, INDEXING, *((int *)key->value));
    }
    printf("query doc is!!!!key->type=%d\n" , key->type);
}

int db_object_restore(db_obj_t *obj)
{
    bson_t query;
    bson_t fields;
    int ret;
    const bson_t *doc; 
    char *_json;
    mongoc_client_t * client; 
    mongoc_collection_t  * collection; 
    mongoc_cursor_t *cursor;
    bson_error_t error; 
    PyObject * value = obj->obj;

    bson_init(&query);
    bson_init(&fields);

    init_query_bson(&obj->db_key, &query);
    BSON_APPEND_INT32(&fields, INDEXING, 0);

    _json = bson_as_json(&query, NULL);
    printf("query doc is %s\n", _json);

    client = mongoc_client_pool_pop(g_client_pool); 
    collection = mongoc_client_get_collection(client, "q1", "dat");

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE,0,1,0,&query,&fields,NULL); 

    if (mongoc_cursor_next(cursor, &doc)) {
        _json = bson_as_json(doc, NULL);
        printf("doc is %s\n", _json);
        if(bson_to_pydict(doc, value)) {
            ret = STATE_OK; 
            PyObject_Print(value, stdout, 0);
            printf("pydict is \n");
        } else {
            log_error("dbd bson to pydict fail!!");
            ret = STATE_ERROR; 
        }    
    }
    else if(mongoc_cursor_error(cursor, &error)) {
        ret = STATE_ERROR; 
        LOG_MONGOC_ERROR(error);
        log_info("common load db obj is errr!");
    }
    else {
        ret = STATE_NULL; 
        log_info("common load db obj is null!");
    }

    bson_destroy(&query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);  
    mongoc_client_pool_push(g_client_pool,client); 
    return ret; 
}

static int db_obj_load(void *buf, size_t len, int sync, marshal_array_t *arr)
{
    marshal_array_iter_t iter;
    FS_DBI_ARRAY_ITER_FIRST(&iter, buf, len);

    int state;
    db_key_t db_key;
    unmarshal_db_key(&iter, &db_key);
    PyObject *dictob = NULL;

    printf("%s value=%s, len=%d\n", __FUNCTION__, (char *)db_key.value, db_key.len);
   
    db_obj_t *obj = (db_obj_t*)g_hash_table_lookup(g_db_object_table, &db_key);
    if (obj == NULL) {
        obj = db_object_new(&db_key);
        assert(obj);
        log_info("start restore db obj! keyvalue=%s,type=%d, len=%d\n", obj->db_key.value, obj->db_key.type, obj->db_key.len);
        log_info("address key=%p,address obj=%p\n", &obj->hostid, obj);
        state = db_object_restore(obj);
        if (state == STATE_OK) {
            g_hash_table_insert(g_db_object_table , &obj->db_key, obj);
            dictob = obj->obj;
        } else {
            db_object_free(obj);
            if (state == STATE_ERROR) {
                log_error("load db object error.\n");
            } else if (state == STATE_NULL) {
                log_warning("load db object null, try new db obj.\n");
            }
        }
    } else {
        state = STATE_OK;
    }
    
    /*
    assert(obj != NULL);
    PyObject_Print(obj->obj, stdout, 0);
    */

    marshal_db_key(arr, &db_key);
    if (marshal(dictob, arr) == -1) {
        state = STATE_ERROR;
    }

    log_info("common load db obj cmd state=%d!", state);
    return state; 
}

//同步加载一个存盘数据
void on_recv_db_obj_sync_load_cmd(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    dg_msg_header_t hdr; 
    buffer_t *buf = NULL;

    marshal_array_t arr; 
    marshal_array_construct(&arr);

    log_info("load db obj begin!");
    int stat = db_obj_load(msgdata, msglen, 1, &arr);

    memset(&hdr, 0, sizeof(hdr));
    hdr.cmd = CMD_DBD_TO_GAMED_DB_OBJ_DATA;
    hdr.state = stat;
    hdr.sid = header->sid;

    buf = &arr.buf;
    hdr.paylen = buf->data_size;

    connection_send(g_gamed_conn_sync, &hdr, sizeof(hdr));
    connection_send_buffer(g_gamed_conn_sync, buf);

    marshal_array_destruct(&arr);
    log_info("dbd send db obj data success\n!");
}

void on_recv_db_obj_async_load_cmd(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    dg_msg_header_t hdr; 
    buffer_t *buf = NULL;

    marshal_array_t arr; 
    marshal_array_construct(&arr);

    log_info("async load db obj begin!");
    int stat = db_obj_load(msgdata, msglen, 0, &arr);

    memset(&hdr, 0, sizeof(hdr));
    hdr.cmd = CMD_DBD_TO_GAMED_DB_OBJ_DATA;
    hdr.state = stat;
    hdr.sid = header->sid;

    buf = &arr.buf;
    hdr.paylen = buf->data_size;

    connection_send(g_gamed_conn, &hdr, sizeof(hdr));
    connection_send_buffer(g_gamed_conn, buf);

    marshal_array_destruct(&arr);
    log_info("dbd send async db obj data success\n!");
}

int unmarshal_obj_all(db_obj_t *obj, marshal_array_iter_t *iter)
{
    PyObject *newobj = unmarshal(iter);
    printf("show new val .\n");
    PyObject_Print(newobj, stdout, 0);
    printf("unmarshal obj data.\n");

    PyDict_Update(obj->obj, newobj);
    Py_DECREF(newobj);
}

void check_strcat_path(PyObject *keyobj, char *path)
{
    int num , len; 
    char * key; 

    static char buf[MAX_KEY_LEN]; 
    memset(buf, 0, MAX_KEY_LEN); 

    if (PyLong_CheckExact(keyobj)) {
        sprintf(buf, I_FORMAT, (int)PyLong_AS_LONG(keyobj)); 
        key = buf; 
    }

    else if (PyUnicode_CheckExact(keyobj)) {
        key = PyUnicode_AsUTF8(keyobj);
        len = strlen(key);
        if (len >=MAX_KEY_LEN) {
            printf("lpc key too large:<<%s>>\n", key); 
            fflush(NULL); 
#ifdef DEVELOP_MODE
            assert(NULL); 
#endif
        }

        if(strchr(key, '.')) {
            printf("invalid string key :<<%s>> containing dot\n", key);
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
    }
    else {
        assert(NULL); 
    }

    if (strlen(path)) {
        strcat(path, ".");
    }
    strcat(path, key); 
}

static PyObject *get_directed_node(PyObject *node, PyObject *key)
{
    MY_PYOBJECT_PRINT(node, "get directed node");
    MY_PYOBJECT_PRINT(key, "get directed node");
    if (PyDict_CheckExact(node)) {
        printf("get_directed_node pathlen\n");
        return PyDict_GetItem(node, key);
    } else if (PyList_CheckExact(node)) {
        printf("get_directed_node 111pathlen\n");
        return PyList_GET_ITEM(node, PyLong_AsSsize_t(key));
    } else {
        log_error("%s:%d except DirtyDict or DirtyList, given: %s", __FILE__, __LINE__, Py_TYPE(key)->tp_name);
        return NULL;
    }
}

static PyObject *get_path_node(PyObject *node, marshal_array_iter_t *iter, char *path)
{
    int i, pathlen;
    PyObject *key, **keys;

    pathlen = iter->tv->arr.cnt;
    FS_DBI_ARRAY_ITER_NEXT(iter);
    printf("get_path_node pathlen=%d\n", pathlen);

    keys = (PyObject **)malloc(sizeof(PyObject *) * pathlen);

    for (i = 0; i < pathlen; ++i) {
        key = unmarshal(iter);
        keys[pathlen - i - 1] = key;
    }

    for (i = 0; i < pathlen; ++i) {
        key = keys[i];
        MY_PYOBJECT_PRINT(key, "get path node print");
        check_strcat_path(key, path);
        node = get_directed_node(node, key);
        Py_DECREF(key);
        if (node == NULL) {
            log_error("get path node error. path:%s", path); 
            return NULL;
        }
    }

    free((void **)keys);
    return node;
}

void bson_set(const char *keypath, PyObject *value, bson_t *set)
{
    if (PyDict_CheckExact(value)) {
        bson_t child; 
        BSON_APPEND_DOCUMENT_BEGIN(set, keypath, &child); 
        pydict_to_bson(value, &child);
        bson_append_document_end(set, &child); 
    }

    else if (PyList_CheckExact(value)) {
        bson_t child;
        BSON_APPEND_ARRAY_BEGIN(set, keypath, &child); 
        pylist_to_bson(value, &child);
        bson_append_array_end(set, &child); 
    }

    else if (PyFloat_CheckExact(value)) {
        BSON_APPEND_DOUBLE(set, keypath, PyFloat_AS_DOUBLE(value));
    }

    else if (PyLong_CheckExact(value)) {
        BSON_APPEND_INT32(set, keypath, PyLong_AS_LONG(value));
    }

    else if (PyUnicode_CheckExact(value)) {
        char *str = PyUnicode_AsUTF8(value); 
#ifdef  DEVELOP_MODE
        if ((!bson_utf8_validate(str,strlen(str),false))) {
            printf("NOT UTF8 keypath:%s, value:%s.\n", keypath, str); 
            fflush(NULL); 
            assert(NULL); 
        }
#endif
        BSON_APPEND_UTF8(set, keypath, str);
    }

    else { 
            log_error("_unsupported type:%s", Py_TYPE(value)->tp_name); 
#ifdef  DEVELOP_MODE
            assert(0);
#endif
    }
}

int unmarshal_dirty_node(PyObject *node, marshal_array_iter_t *iter, bson_t *set, bson_t *unset, const char *path)
{
    char keypath[MAX_KEY_LEN] = {0};
    PyObject *key, *value;

    int pair = iter->tv->map.pair;
    FS_DBI_ARRAY_ITER_NEXT(iter);

    for (int i = 0; i < pair; i++)  
    {
        memset(keypath, 0, MAX_KEY_LEN);
        strcat(keypath, path);

        key = unmarshal(iter);
        check_strcat_path(key, keypath);
        value = unmarshal(iter);

        if (value == Py_None)  {
            PyDict_DelItem(node, key);
            BSON_APPEND_UTF8(unset,keypath,"");
        } else {
            PyDict_SetItem(node, key, value);
            bson_set(keypath, value, set);
        }
    }
}

int unmarshal_obj_dirty(db_obj_t *obj, marshal_array_iter_t *iter, bson_t *set, bson_t *unset)
{
    printf("unmarshal obj dirty.\n");
    char path[MAX_CAT_KEY_LEN] = {0};
    PyObject *node;
    bson_t set_child;
    bson_t unset_child;

    bson_init(&set_child);
    bson_init(&unset_child);

    int cnt = iter->tv->arr.cnt;
    printf("unmarshal obj dirty pair=%d\n", cnt);
    FS_DBI_ARRAY_ITER_NEXT(iter);

    MY_PYOBJECT_PRINT(obj->obj, "unmarshal obj dirty");

    for (int i = 0; i < cnt; i++)  {
        memset(path, 0, MAX_CAT_KEY_LEN);
        node = get_path_node(obj->obj, iter, path);
        printf("unmarshal obj dirty path=%s\n", path);
        if (node == NULL) {
            fprintf(stderr, "%s:%d get_node null path=%s\n", __FILE__, __LINE__, path);
            return -1;
        }
        unmarshal_dirty_node(node, iter, &set_child, &unset_child, path);
    }

    BSON_APPEND_DOCUMENT(set,"$set",&set_child); 
    BSON_APPEND_DOCUMENT(unset,"$unset",&unset_child);

    return 0;
}

int save_db_obj_all(db_obj_t *obj, db_key_t *dbkey)
{
    bson_error_t error;
    mongoc_client_t * client; 
    mongoc_collection_t *collection;
    bson_t selector;
    bson_t update;

    bson_init(&selector);
    bson_init(&update);

    pydict_to_bson(obj->obj, &update);
    if (dbkey->type == STRING_TYPE) {
        BSON_APPEND_UTF8(&selector,INDEXING, (char *)dbkey->value);   
        BSON_APPEND_UTF8(&update,INDEXING,(char *)dbkey->value);
    }    
    else {
        BSON_APPEND_INT32(&selector,INDEXING,*((int *)dbkey->value));  
        BSON_APPEND_INT32(&update,INDEXING,*((int *)dbkey->value));    
    }    

    client = mongoc_client_pool_pop(g_client_pool); 
    collection = mongoc_client_get_collection(client,"q1","dat"); 
    if (!mongoc_collection_update(collection,MONGOC_UPDATE_UPSERT,&selector,&update,NULL,&error)) {
        printf("mongod server status error. code=%d,message=%s\n", error.code, error.message);
    }

    printf("update db success.\n");

    mongoc_collection_destroy(collection);  
    mongoc_client_pool_push(g_client_pool, client); 
}

int save_db_obj_dirty(db_obj_t *obj, db_key_t *dbkey, bson_t *set, bson_t *unset)
{
	bson_error_t error; 
	mongoc_client_t * client; 
    bson_t selector;
	mongoc_collection_t  *collection; 

    bson_init(&selector);

    if (dbkey->type == STRING_TYPE) {
        BSON_APPEND_UTF8(&selector,INDEXING, (char *)dbkey->value);   
    }    
    else {
        BSON_APPEND_INT32(&selector,INDEXING,*((int *)dbkey->value));  
    }    

    client = mongoc_client_pool_pop(g_client_pool); 
	collection = mongoc_client_get_collection(client, "q1", "dat"); 

	if (!bson_empty(set)) {
		if(!mongoc_collection_update(collection,MONGOC_UPDATE_NONE,&selector,set,NULL,&error)) {
			LOG_MONGOC_ERROR(error); 
			QTZ_BSON_PRINT(&selector, "show select bson");
			QTZ_BSON_PRINT(set, "show set bson");
		}

		bson_clear(&set); 
	}

	if (!bson_empty(unset)) {
		if(!mongoc_collection_update(collection,MONGOC_UPDATE_NONE,&selector,unset,NULL,&error)) {
			LOG_MONGOC_ERROR(error) ; 
			QTZ_BSON_PRINT(&selector, "show select bson");
			QTZ_BSON_PRINT(unset, "show unset bsson");
		}

		bson_clear(&unset); 
	}

	mongoc_collection_destroy(collection);  
    mongoc_client_pool_push(g_client_pool, client); 
}

void on_recv_db_obj_save_cmd(gd_msg_header_t *header, void *msgdata, int msglen, void *arg, db_save_method_t method)
{
    db_key_t db_key;
    marshal_array_t arr; 
    marshal_array_iter_t iter;
    bson_t set;
    bson_t unset;

    marshal_array_construct(&arr);
    bson_init(&set);
    bson_init(&unset);

    FS_DBI_ARRAY_ITER_FIRST(&iter, msgdata, msglen);

    log_info("on recv db obj save cmd!");

    unmarshal_db_key(&iter, &db_key);
    FS_DBI_ARRAY_ITER_NEXT(&iter);

    db_obj_t *dbobj = (db_obj_t*)g_hash_table_lookup(g_db_object_table, &db_key);
    if (dbobj == NULL) {
        log_error("missing db key! on save docname=%s,type=%d, len=%d,strlen=%d\n", 
            db_key.value, db_key.type, db_key.len, strlen(db_key.value));
        return;
    }

    if (method == SAVE_DATA) { 
        unmarshal_obj_all(dbobj, &iter);
        save_db_obj_all(dbobj, &db_key);
    } else if (method == SAVE_DIRTY) {
        if (unmarshal_obj_dirty(dbobj, &iter, &set, &unset) == -1) {
            log_error("unmarshal obj dirty error\n");
            return;
        }
        save_db_obj_dirty(dbobj, &db_key, &set, &unset);
        QTZ_BSON_PRINT(&set,"");
        QTZ_BSON_PRINT(&unset,"");
    } else {
        assert(0);
    }

    marshal_array_destruct(&arr);
    log_info("dbd send async db obj data success\n!");
}


void on_recv_db_obj_new_cmd(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    db_key_t db_key;
    marshal_array_t arr; 
    marshal_array_iter_t iter;
    db_obj_t *dbobj = NULL;

    marshal_array_construct(&arr);

    FS_DBI_ARRAY_ITER_FIRST(&iter, msgdata, msglen);

    unmarshal_db_key(&iter, &db_key);
    FS_DBI_ARRAY_ITER_NEXT(&iter);

    dbobj = (db_obj_t*)g_hash_table_lookup(g_db_object_table, &db_key);
    if (dbobj != NULL) {
        log_error("new db obj fail, is exist\n");
        return;
    }
    dbobj = db_object_new(&db_key);
    assert(dbobj);
    g_hash_table_insert(g_db_object_table , &dbobj->db_key, dbobj);
    save_db_obj_all(dbobj, &db_key);

    log_info("on recv_sync_load_db_msg cmd !");
    marshal_array_destruct(&arr);
}

void on_recv_db_obj_unload_cmd(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    db_key_t db_key;
    marshal_array_t arr; 
    marshal_array_iter_t iter;
    db_obj_t *dbobj = NULL;

    marshal_array_construct(&arr);

    FS_DBI_ARRAY_ITER_FIRST(&iter, msgdata, msglen);

    unmarshal_db_key(&iter, &db_key);

    dbobj = (db_obj_t*)g_hash_table_lookup(g_db_object_table, &db_key);
    if (dbobj == NULL) {
        log_error("unload db obj fail, not online\n");
        return;
    }
    g_hash_table_remove (g_db_object_table,&db_key);
    db_object_free(dbobj);
    log_info("on recv_sync_load_db_msg cmd !");
    marshal_array_destruct(&arr);
}

//---------------------------------------------------
static void gamed_msg_dispatch(void *arg, gd_msg_header_t *header, void *msgdata, size_t msglen)
{
    int cmd = header->cmd;
    switch (cmd) {
        case CMD_GAMED_TO_DBD_DB_OBJ_NEW:
            on_recv_db_obj_new_cmd(header, msgdata, msglen, arg);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_LOAD_SYNC:
            on_recv_db_obj_sync_load_cmd(header, msgdata, msglen, arg);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC:
            on_recv_db_obj_async_load_cmd(header, msgdata, msglen, arg);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DATA:
            on_recv_db_obj_save_cmd(header, msgdata, msglen, arg, SAVE_DATA);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DIRTY:
            on_recv_db_obj_save_cmd(header, msgdata, msglen, arg, SAVE_DIRTY);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_UNLOAD:
            on_recv_db_obj_unload_cmd(header, msgdata, msglen, arg);
            break;
        default:
            log_error("recv gamed unknown cmd! cmd=%d", cmd);
            break;
    }
}

static void on_gamed_connection_recv(struct bufferevent *bufev, void *arg)
{
    READ_PACKET(gd_msg_header_t, bufev, arg, gamed_msg_dispatch);
}

static void on_gamed_connection_write(struct bufferevent *bufev, void *arg)
{
    log_info("on_gamed_connection_write! evbuffer_get_length=%ld", evbuffer_get_length(bufferevent_get_output(bufev)));
}

static void on_gamed_connection_event(struct bufferevent *bufev, short what, void *arg)
{
    log_info("on_gamed_connection_event!what=%x", what);

    if (what & BEV_EVENT_EOF) {
        log_info("connection close");
        connection_free(g_gamed_conn);
        connection_free(g_gamed_conn_sync);
        g_gamed_conn = NULL;
        g_gamed_conn_sync = NULL;
    } else if (what & BEV_EVENT_ERROR) {
        log_info("got an error on the connection: %s", strerror(errno));
    } else if (what & BEV_EVENT_READING) {
        log_info("error occur when reading");
    } else if (what & BEV_EVENT_TIMEOUT) {
        log_info("error occur timeout");
    } else if (what & BEV_EVENT_WRITING) {
        log_info("error occur when writing");
    } else if (what & BEV_EVENT_CONNECTED) {
        log_info("error occur connected !!!");
    }
}

static void on_gamed_accept(evutil_socket_t listenfd, short what, void * udata)
{
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    struct sockaddr_in dstaddr;
    socklen_t dstsocklen = sizeof(dstaddr);

    int connfd = accept(listenfd, (struct sockaddr*)&addr, &socklen);
    if (connfd < 0) {
        perror("game connect accept error");
        return;
    }  

    log_info("on gamed accpet! connfd=%d", connfd);
    if (g_gamed_conn == NULL) {
        g_gamed_conn = connection_new(ev_base, connfd, on_gamed_connection_recv, 
                on_gamed_connection_write, on_gamed_connection_event, NULL);
    } else if (g_gamed_conn_sync == NULL) {
        g_gamed_conn_sync = connection_new(ev_base, connfd, on_gamed_connection_recv, 
                on_gamed_connection_write, on_gamed_connection_event, NULL);
    } else {
        log_warning("on gamed accpet too much.");
    }
}

void init_connect()
{
    int listenfd = net_listen("127.0.0.1", 6666, 1024, 0);
    acceptor_t* acceptor = acceptor_new(ev_base, listenfd, on_gamed_accept, NULL);
}

void dbd_init()
{
    init_mongod();
    init_connect();

    g_db_object_table = g_hash_table_new(db_object_hash_fun, db_object_equal_fun);

    log_info("dbd init success!");
}

void dbd_startup()
{
    log_info("dbd startup success!");
}


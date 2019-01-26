#include <bson.h>
#include <assert.h>
#include <mongoc.h>
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
        return strcmp(key1->value, key2->value) == 0;
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
        printf("db_key_init value=%s, len=%d, key=%s,keylen=%d\n", (char *)value, len, (char *)key->value, key->len);
    }
}

db_object_t *db_object_new(db_key_t *key)
{
    db_object_t *dbo = (db_object_t *)calloc(1, sizeof(db_object_t));
    db_key_init(&dbo->db_key, key->type, key->len, key->value);
    dbo->obj = PyDict_New();
    return dbo;
}

void init_mongod()
{
    bson_t reply;
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
}

void unmarshal_db_key(marshal_array_iter_t *iter, db_key_t *key)
{
    key->type = iter->tv->number.n;
    FS_DBI_ARRAY_ITER_NEXT(iter);
    if (key->type == INT_TYPE) {
        key->value = &iter->tv->number.n;
        key->len = sizeof(iter->tv->number.n);
    } else if (key->type == STRING_TYPE) {
        key->value = iter->tv->string.str;
        key->len = iter->tv->string.len;
    }
}

void marshal_db_key(marshal_array_t *arr, db_key_t *key)
{
    if (key->type == STRING_TYPE) {
        marshal_array_push_int(arr, STRING_TYPE);
        marshal_array_push_lstring(arr, key->value, key->len);
    } else if (key->type == INT_TYPE) {
        marshal_array_push_int(arr, STRING_TYPE);
        marshal_array_push_int(arr, *((int *)key->value));
    }
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

int db_object_restore(db_object_t *obj)
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

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE,0,1,0,&query,&fields,NULL) ; 

    if (mongoc_cursor_next(cursor, &doc)) {
        _json = bson_as_json(doc, NULL);
        printf("doc is %s\n", _json);
        if(bson_to_pydict(doc, value)) {
            ret = STATE_OK; 
            PyObject_Print(value, stdout, 0);
            printf("pydict is \n");
        } else {
            log_error("dbd sync socket close");
            ret = STATE_ERROR; 
        }    
    }
    else if(mongoc_cursor_error(cursor, &error)) {
        ret = STATE_ERROR; 
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

static int common_dbo_load(void *buf, size_t len, int sync, marshal_array_t *arr)
{
    marshal_array_iter_t iter;
    FS_DBI_ARRAY_ITER_FIRST(&iter, buf, len);

    int cmd_res;
    db_key_t db_key;
    unmarshal_db_key(&iter, &db_key);

    printf("%s value=%s, len=%d\n", __FUNCTION__, (char *)db_key.value, db_key.len);
   
    db_object_t *obj = (db_object_t*)g_hash_table_lookup(g_db_object_table, &db_key);
    if (obj == NULL) {
        obj = db_object_new(&db_key);
        assert(obj);
        log_info("start restore db obj!");
        cmd_res = db_object_restore(obj);
        g_hash_table_insert(g_db_object_table , &obj->db_key, obj);
    } else {
        cmd_res = CMD_OK;
    }
    
    assert(obj != NULL);
    PyObject_Print(obj->obj, stdout, 0);

    marshal_db_key(arr, &db_key);
    if (marshal(obj->obj, arr) == -1) {
        return CMD_ERR;
    }

    log_info("common load db obj cmd res=%d!", cmd_res);
    return cmd_res; 
}

//同步加载一个存盘数据
void load_db_obj_sync(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    dg_msg_header_t hdr; 
    buffer_t *buf = NULL;

    marshal_array_t arr; 
    marshal_array_construct(&arr);

    log_info("load db obj begin!");
    int stat = common_dbo_load(msgdata, msglen, 1, &arr);

    log_info("load db obj end!");

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

void load_db_obj_async(gd_msg_header_t *header, void *msgdata, int msglen, void *arg)
{
    dg_msg_header_t hdr; 
    buffer_t *buf = NULL;

    marshal_array_t arr; 
    marshal_array_construct(&arr);

    log_info("async load db obj begin!");
    int stat = common_dbo_load(msgdata, msglen, 0, &arr);

    log_info("async load db obj end!");

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

static void gamed_msg_handler(void *arg, gd_msg_header_t *header, void *msgdata, size_t msglen)
{
    int cmd = header->cmd;
    switch(cmd) {
        case CMD_GAMED_TO_DBD_DB_OBJ_LOAD_SYNC:
            load_db_obj_sync(header, msgdata, msglen, arg);
            break;
        case CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC:
            load_db_obj_async(header, msgdata, msglen, arg);
            break;
        default:
            log_error("recv gamed unknown cmd! cmd=%d", cmd);
            break;
    }
}

static void on_gamed_connection_recv(struct bufferevent *bufev, void *arg)
{
    READ_PACKET(gd_msg_header_t, bufev, arg, gamed_msg_handler);
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


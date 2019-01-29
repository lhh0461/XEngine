//

#include <unistd.h>
#include <fcntl.h>
#include <Python.h>

#include "network.h"
#include "common.h"
#include "log.h"
#include "db.h"
#include "dirty.h"
#include "protocol_dbd.h"
#include "util.h"
#include "buffer.h"
#include "marshal.h"

extern connection_t *g_dbd_connection;
extern int g_sync_dbd_fd;

void unmarshal_db_key(marshal_array_iter_t *iter, db_key_t *dbkey)
{
    dbkey->type = iter->tv->number.n;
    FS_DBI_ARRAY_ITER_NEXT(iter);
    if (dbkey->type == INT_TYPE) {
        dbkey->value = &iter->tv->number.n;
        dbkey->len = sizeof(iter->tv->number.n);
    } else if (dbkey->type == STRING_TYPE) {
        dbkey->value = iter->tv->string.str;
        dbkey->len = iter->tv->string.len;
    }
}

void marshal_db_key(marshal_array_t *arr, db_key_t *dbkey)
{
    if (dbkey->type == STRING_TYPE) {
        marshal_array_push_int(arr, STRING_TYPE);
        marshal_array_push_string(arr, dbkey->value);
    } else if (dbkey->type == INT_TYPE) {
        marshal_array_push_int(arr, INT_TYPE);
        marshal_array_push_int(arr, *((int *)dbkey->value));
    }
}

void marshal_doc_id(marshal_array_t *arr, PyObject *docid)
{
    if (PyLong_CheckExact(docid)) {
        marshal_array_push_int(arr, INT_TYPE);
        marshal_array_push_int(arr, PyLong_AsLong(docid));
    } else if (PyUnicode_CheckExact(docid)) {
        marshal_array_push_int(arr, STRING_TYPE);
        marshal_array_push_string(arr, PyUnicode_AsUTF8(docid));
    } else {
        assert(0);
    }
}

static int send_sync_load_db_msg(connection_t *conn, gd_msg_header_t *hdr, buffer_t *buf)
{
    int flag;
    struct evbuffer *outevb = bufferevent_get_output(conn->bufev);

    evbuffer_unfreeze(outevb, 0); 

    if (buf == NULL) {
        hdr->paylen = 0;
        evbuffer_add(outevb, hdr, sizeof(*hdr));
    } else {
        hdr->paylen = buf->data_size;
        evbuffer_add(outevb, hdr, sizeof(*hdr));
        buffer_to_evbuffer(outevb, buf);
    }

    evbuffer_unfreeze(outevb, 1); 
    set_block(conn->fd, 1, &flag);

    int left, ret;
    //fprintf(stderr, "send msg len=%ld\n", evbuffer_get_length(outevb));
    while ((left = evbuffer_get_length(outevb)) > 0) {
        ret = evbuffer_write(outevb, conn->fd);
        if (ret < 0) {
            evbuffer_drain(outevb, left);
            fprintf(stderr, "evbuffer write errormsg:%s\n", strerror(errno));
            fcntl(conn->fd, F_SETFL, flag);
            return -1;
        }
    }

    assert(evbuffer_get_length(outevb) == 0);
    fprintf(stderr, "send sync load db msg\n");

    fcntl(conn->fd, F_SETFL, flag);
    return 0;
}

struct evbuffer *recv_sync_load_db_msg(int fd)
{
    struct evbuffer *inevb = evbuffer_new();
    int ret = 0;				
    size_t packet_sz = 0;				
    dg_msg_header_t *header = NULL;			
    size_t bufsz = 1024;
    char buf[bufsz];

    fprintf(stderr, "recv sync data from gamed\n");
    while (1) {		
        ret = read(fd, buf, bufsz); 
        //读取出错
        if (ret < 0) {
            perror("syscall db sync read error");
            //read收到信号打断
            if (errno == EINTR) {
                fprintf(stderr, "inter sync recv\n");
                continue;
            }
            return NULL;
        } else if (ret == 0) {
            //对端tcp连接关闭
            fprintf(stderr, "dbd sync socket close\n");
            INTERNAL_ASSERT(0);
            return NULL;
        } else {
            evbuffer_add(inevb, buf, ret);
            if (evbuffer_get_length(inevb) < sizeof(dg_msg_header_t))  					
                continue;

            if (header == NULL) {
                header = (dg_msg_header_t *)evbuffer_pullup(inevb,-1);	
                packet_sz = sizeof(dg_msg_header_t) + header->paylen;		
            }

            if (evbuffer_get_length(inevb) >= packet_sz)
                break;
        }
    }

    fprintf(stderr, "sync recv from dbd ok\n");
    INTERNAL_ASSERT(evbuffer_get_length(inevb) == packet_sz);
    return inevb;
}


PyObject *recv_db_obj(dg_msg_header_t *header, void *msgdata, size_t msglen)
{
    marshal_array_iter_t iter;
    int stat;
    db_key_t db_key;


    FS_DBI_ARRAY_ITER_FIRST(&iter, msgdata, msglen);

    //printf("db object recv len=%d!!!!\n", FS_DBI_ARRAY_ITEM_SIZE(&iter));

    unmarshal_db_key(&iter, &db_key);
    FS_DBI_ARRAY_ITER_NEXT(&iter);

    PyObject *obj = unmarshal(&iter);
    if (!obj) {
        printf("db object recv null!!!!\n");
        return NULL;
    }
    //PyObject_Str(obj);
    
    printf("recv db obj data!!!!\n");
    PyObject_Print(obj, stdout, 0);
    printf("\n");

    return obj;
}

PyObject *load_db_object_sync(connection_t *conn, gd_msg_header_t *header, buffer_t *buf, int *state)
{
    if (send_sync_load_db_msg(conn, header, buf) == 0) {
        struct evbuffer *evbuf = recv_sync_load_db_msg(g_sync_dbd_fd);
        if (evbuf != NULL) {
            dg_msg_header_t *header = (dg_msg_header_t *)evbuffer_pullup(evbuf, -1);
            char *msgdata = header->paylen == 0 ? NULL : (char* )(header + 1);
            printf("on load db object sync paylen=%d\n", header->paylen);
            if (header->cmd == CMD_DBD_TO_GAMED_DB_OBJ_DATA) {
                *state = header->state;
                if (header->state == STATE_OK) {
                    return recv_db_obj(header, msgdata, header->paylen);
                } else if (header->state == STATE_ERROR) {
                    log_error("sync load db object error.");
                    return NULL; 
                }
            }
        }
    }
    return NULL;
}

PyObject *load_db_obj_sync(PyObject *docid, int sid, int *state)
{
    PyObject *obj = NULL; 
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_doc_id(&arr, docid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_SYNC;
    header.paylen = marshal_array_datalen(&arr);
    header.sid = sid;

    obj = load_db_object_sync(g_dbd_connection, &header, &arr.buf, state);

    marshal_array_destruct(&arr);
    return obj;
}

int load_db_obj_async(PyObject *docid, int sid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_doc_id(&arr, docid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC;
    header.paylen = marshal_array_datalen(&arr);
    header.sid = sid;

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    log_info("send async load db obj cmd success!");
    marshal_array_destruct(&arr);
    return 0;
}

int save_db_obj(PyObject *docid, PyObject *obj, int save_method)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_doc_id(&arr, docid);

    MY_PYOBJECT_PRINT(docid, "save db obj doc id");
    MY_PYOBJECT_PRINT(obj, "save db obj doc obj");

    if (save_method == SAVE_DATA) {
        header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DATA;
        if (marshal(obj, &arr) == -1) return -1;
    } else {
        header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DIRTY;
        if (marshal_dirty(obj, &arr) == -1) return -1;;
    }

    clear_dirty(obj);

    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
    return 0;
}

PyObject *new_db_object(PyObject *docid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;
    marshal_array_construct(&arr);

    memset(&header, 0, sizeof(header));

    marshal_doc_id(&arr, docid);

    PyObject *newobj = PyDict_New();
    if (newobj == NULL) {
        return NULL;
    }

    marshal(newobj, &arr);
    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_NEW;
    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);
    printf("on new_db_obj\n ");
    
    marshal_array_destruct(&arr);
    return newobj;
}

int unload_db_obj(PyObject *docid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_doc_id(&arr, docid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_UNLOAD;
    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
    return 0;
}

//

#include <unistd.h>
#include <fcntl.h>
#include <Python.h>

#include "network.h"
#include "common.h"
#include "log.h"
#include "db.h"
#include "protocol_dbd.h"
#include "util.h"
#include "buffer.h"
#include "marshal.h"

extern connection_t *g_dbd_connection;
extern int g_sync_dbd_fd;

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
    fprintf(stderr, "send msg len=%ld\n", evbuffer_get_length(outevb));
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


PyObject *db_object_recv(dg_msg_header_t *header, void *data, size_t len)
{
    marshal_array_iter_t iter;
    int stat;
    db_key_t db_key;

    FS_DBI_ARRAY_ITER_FIRST(&iter, data, len);

    unmarshal_db_key(&iter, &db_key);
    FS_DBI_ARRAY_ITER_NEXT(&iter);

    PyObject *obj = unmarshal(&iter);
    if (!obj) {
        printf("db object recv null!!!!\n");
        return NULL;
    }
    PyObject_Print(obj, stdout, 0);
    printf("print original data!!!!\n");

    return obj;
}

PyObject *new_db_object(dg_msg_header_t *header, void *data, size_t len)
{

}

PyObject *load_db_object_sync(connection_t *conn, gd_msg_header_t *header, buffer_t *buf)
{
    if (send_sync_load_db_msg(conn, header, buf) == 0) {
        struct evbuffer *evbuf = recv_sync_load_db_msg(g_sync_dbd_fd);
        if (evbuf != NULL) {
            dg_msg_header_t *header = (dg_msg_header_t *)evbuffer_pullup(evbuf, -1);
            char *data = header->paylen == 0 ? NULL : (char* )(header + 1);
            printf("on load db object sync paylen=%d\n", header->paylen);
            if (data) {
                if (header->cmd == CMD_DBD_TO_GAMED_DB_OBJ_DATA) {
                    if (header->state == STATE_OK) {
                        return db_object_recv(header, data, header->paylen);
                    } else if (header->state == STATE_NULL) {
                        return new_db_object(header, data, header->paylen);
                    } else if (header->state == STATE_ERROR) {
                        return NULL; 
                    }
                }
            }
        }
    }
    return NULL;
}

PyObject *load_dat_sync(const char * docname, int sid)
{
    PyObject *obj = NULL; 
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, STRING_TYPE);
    marshal_array_push_string(&arr, docname);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_SYNC;
    header.paylen = marshal_array_datalen(&arr);
    header.sid = sid;

    obj = load_db_object_sync(g_dbd_connection, &header, &arr.buf);

    marshal_array_destruct(&arr);
    return obj;
}

int load_dat_async(const char * docname, int sid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, STRING_TYPE);
    marshal_array_push_string(&arr, docname);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC;
    header.paylen = marshal_array_datalen(&arr);
    header.sid = sid;

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    log_info("send async load db obj cmd success!");
    marshal_array_destruct(&arr);
    return 0;
}

void load_user_sync(int uid, int sid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, sid);
    marshal_array_push_int(&arr, INT_TYPE);
    marshal_array_push_int(&arr, uid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC;
    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
}

int load_user_async(int uid, int sid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, sid);
    marshal_array_push_int(&arr, INT_TYPE);
    marshal_array_push_int(&arr, uid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC;
    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
    return 0;
}

int save_user(int uid)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, sid);
    marshal_array_push_int(&arr, INT_TYPE);
    marshal_array_push_int(&arr, uid);

    header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_SAVE;
    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
    return 0;
}

void marshal_db_obj_dirty(marshal_array_t *arr, PyDirtyDictObject *ob)
{

}

void marshal_db_obj_data(marshal_array_t *arr, PyDirtyDictObject *ob)
{
    
}

int save_dat(const char *docname, PyDirtyDictObject *ob, int save_method)
{
    marshal_array_t arr; 
    gd_msg_header_t header;

    memset(&header, 0, sizeof(header));
    marshal_array_construct(&arr);

    marshal_array_push_int(&arr, STRING_TYPE);
    marshal_array_push_string(&arr, docname);

    if (save_method == SAVE_DATA) {
        header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DATA;
        marshal_db_obj_dirty(&arr, ob);
    } else {
        header.cmd = CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DIRTY;
        marshal_db_obj_data(&arr, ob);
    }

    header.paylen = marshal_array_datalen(&arr);

    connection_send(g_dbd_connection, &header, sizeof(header));
    connection_send_buffer(g_dbd_connection, &arr.buf);

    marshal_array_destruct(&arr);
    return 0;
}


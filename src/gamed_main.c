#include <Python.h>

#include "protocol_dbd.h"
#include "start_up.h"
#include "network.h"
#include "dirty.h"
#include "script.h"
#include "common.h"
#include "db.h"

int g_sync_dbd_fd;
connection_t * g_dbd_connection;

static void on_recv_db_obj_data(void *arg, dg_msg_header_t *header, void *data, size_t len)
{
    PyObject * args;
    PyObject * obj;
    int sid = header->sid;
    int state = header->state;
    obj = recv_db_obj(header, data, len);
    args = Py_BuildValue("iOi", header->state, obj, sid);
    PyObject_Print(args, stdout, 0);
    printf("on recv dbd obj data\n ");

/*
    PyDirtyDictObject *dict = build_dirty_dict(obj);
    set_dirty_dict_subdocs(dict, subdocs);

    handle_old_dirty_subdocs(dict);
    begin_dirty_manage_dict(dict, NULL, NULL);
    handle_new_dirty_subdocs(dict);
    */

    call_script_func("db", "on_db_obj_async_load_succ", args);
}

static void dbd_msg_dispatch(void *arg, dg_msg_header_t *header, void *data, size_t len)
{
    int cmd = header->cmd;
    switch(cmd) {
        case CMD_DBD_TO_GAMED_DB_OBJ_DATA:
            on_recv_db_obj_data(arg, header, data, len);
            break;
        default:
            printf("recv dbd unknown cmd! cmd=%d", cmd);
            break;
    }
}

void on_dbd_connection_recv(struct bufferevent *bufev, void *ctx)
{
    READ_PACKET(dg_msg_header_t, bufev, ctx, dbd_msg_dispatch);
}

void on_dbd_connection_write(struct bufferevent *bev, void *ctx)
{
    printf("on dbd write!\n");
}

void on_dbd_connection_error(struct bufferevent *bev, short what, void *ctx)
{
    printf("on dbd connection error!what=%x\n", what);
    if (what & BEV_EVENT_EOF) {
        printf("connection close\n");
    } else if (what & BEV_EVENT_ERROR) {
        printf("got an error on the connection: %s\n", strerror(errno));
    } else if (what & BEV_EVENT_READING) {
        printf("error occur when reading\n");
    } else if (what & BEV_EVENT_TIMEOUT) {
        printf("error occur timeout\n");
    } else if (what & BEV_EVENT_WRITING) {
        printf("error occur when writing\n");
    } else if (what & BEV_EVENT_CONNECTED) {
        printf("error occur connected !!!\n");
    }
}

int connect_to_dbd()
{
    //TODO 从配置读取
    int dbd_fd = net_connect("127.0.0.1", 6666, 1);
    g_dbd_connection = connection_new(ev_base, dbd_fd, on_dbd_connection_recv, on_dbd_connection_write, 
            on_dbd_connection_error, NULL);

    int sync_dbd_fd = net_connect("127.0.0.1", 6666, 1);
    if (sync_dbd_fd < 0) {
        printf("exit for fail to sync connect dbd\n");
    }
    g_sync_dbd_fd = sync_dbd_fd;
}

void gamed_init()
{
    dirty_mem_pool_setup();

    connect_to_dbd();
}

//#include "marshal.h"
//#include "bson_format.h"
void gamed_startup()
{
    //format_test();
    //marshal_test();

    printf("on gamed_startup!!!\n");
    PyObject * pModule = PyImport_ImportModule("db");
    if (pModule == NULL) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", "db");
    }
    printf("on gamed_startup2!!!\n");
    pModule = PyImport_ImportModule("test_dirty");
    if (pModule == NULL) {
        if (PyErr_Occurred()) {
            fprintf(stderr, "Failed to load1111 \"%s\"\n", "test1");
            PyErr_Print();
        }
        fprintf(stderr, "Failed to load \"%s\"\n", "test1");
    }
    printf("on gamed_startup3!!!\n");

    //int res = pack(1, PyObject *obj, msgpack_sbuffer *sbuf)

}

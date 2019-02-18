#ifndef __PROTOCOL_DBD_H__
#define __PROTOCOL_DBD_H__

//between gamed and dbd cmd
enum gd_cmd_e {
    //gamd->dbd
    CMD_GAMED_TO_DBD_DB_OBJ_NEW = 1,
    CMD_GAMED_TO_DBD_DB_OBJ_LOAD_SYNC = 2,
    CMD_GAMED_TO_DBD_DB_OBJ_LOAD_ASYNC = 3,
    CMD_GAMED_TO_DBD_DB_OBJ_UNLOAD = 4,
    CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DIRTY = 5,
    CMD_GAMED_TO_DBD_DB_OBJ_SAVE_DATA = 6,
    CMD_GAMED_TO_DBD_RPC = 7,
    //dbd->gamed
    CMD_DBD_TO_GAMED_DB_OBJ_DATA = 8,
    CMD_DBD_TO_GAME_RPC = 9,
};

//gamd->dbd
typedef struct gd_msg_header_s {
    int cmd; 
    int paylen;
    int sid;
} gd_msg_header_t;

//dbd->gamed
typedef struct dg_msg_header_s {
    int cmd;//命令
    int paylen;
    int sid;
    int state;
} dg_msg_header_t;

#endif //__PROTOCOL_DBD_H__

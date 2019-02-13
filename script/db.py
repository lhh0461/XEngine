#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys 
import os
import log
import threading
import dirty

#常量定义开始
DB_TYPE_USER = 1
DB_TYPE_DAT = 2

#执行结果
OK = 0
ERROR = 1

#引擎脚本各维护一份修改同步修改
STATE_OK = 1
STATE_ERROR = 2
STATE_NULL = 3

#存盘方式
SAVE_ALL = 0
SAVE_DIRTY = 1
#常量定义结束

#全局变量定义开始
#所有存盘对象
all_db_obj = {}

#加载中对象
loading_obj = {}

sid = 0
logger = log.get_logger("db")
#全局变量定义结束

def fun_timer():
    global timer
    print("on fun timer call back")
    timer = threading.Timer(10, fun_timer)
    timer.start()
    dirty.save_all(SAVE_DIRTY) 

timer = threading.Timer(1, fun_timer)
timer.start()

#引擎回调
def on_db_obj_async_load_succ(state, doc, sid):
    if not sid in loading_obj:
        logger.warning("db object async load missing doc id=%d" %(sid))
        return -1

    load_data = loading_obj[sid]
    del loading_obj[sid]
    
    doc_id = load_data["doc_id"]
    db_type = load_data["db_type"]
    subdocs = load_data["subdocs"]
    
    print("on_db_obj_async_load_succ state=%d,doc_id=%s,db_type=%d"%(state,doc_id,db_type))
    if doc_id in all_db_obj:
        logger.warning("async load doc but online docid=%d" %(doc_id))
        load_data["fun"](OK, all_db_obj[doc_id], *load_data["args"])
        return 1
    
    if state == STATE_ERROR:
        if load_data["fun"] != None:
            load_data["fun"](ERROR, None, *load_data["args"])
        return -1
    elif state == STATE_NULL:
        doc = dirty.new_doc(doc_id, subdocs)
        dirty.save_doc(doc_id, doc, SAVE_ALL)
    else:
        doc = dirty.enable_dirty(doc, subdocs)

    all_db_obj[doc_id] = doc
 
    if load_data["fun"] != None:
        load_data["fun"](OK, doc, *load_data["args"])
    
    print("on db object load success!")
    return 1

def load_db_obj(db_type, doc_id, is_async, subdocs, fun=None, args=None):
    #print("start load db obj is_async=%d,doc_id=%s,args=%s,subdocs=%s"%(is_async, doc_id, args, subdocs))
    if doc_id in loading_obj:
        return
    
    global sid
    #已经加载，马上返回
    if doc_id in all_db_obj:
        if is_async:
            if fun is not None:
                fun(OK, all_db_obj[doc_id], args, 1, 1) 
        else:
            return all_db_obj[doc_id]
    
    if is_async:
        sid += 1
        loading_obj[sid] = {
            "db_type":db_type,
            "is_async":is_async,
            "sid":sid,
            "doc_id":doc_id,
            "fun":fun, 
            "args":args,
            "subdocs":subdocs,
        }
        return dirty.async_load(doc_id, sid)
    else:
        doc = dirty.sync_load(doc_id, 1, subdocs)
        all_db_obj[doc_id] = doc
        return all_db_obj[doc_id]

def load_dat_sync(doc_name, subdocs):
    return load_db_obj(DB_TYPE_DAT, doc_name, 0, subdocs)

def load_dat_async(doc_name, subdocs, func, *args):
    return load_db_obj(DB_TYPE_DAT, doc_name, 1, subdocs, func, args)

def load_user_sync(uid, subdocs):
    return load_db_obj(DB_TYPE_UER, uid, 0, subdocs)

def load_user_async(uid, subdocs, func, *args):
    return load_db_obj(DB_TYPE_USER, uid, 1, subdocs, func, args)

def save_all(method):
    dirty.save_all(method) 

def save_doc(doc_id, save_method):
    if doc_id in all_db_obj:
        obj = all_db_obj[doc_id]
        dirty.save_doc(doc_id, obj, save_method)

#存一次盘，释放对象
def unload_doc(doc_id, save_method=0):
    if doc_id in all_db_obj:
        obj = all_db_obj[doc_id]
        del all_db_obj[doc_id]
        dirty.unload(doc_id, obj, save_method) 

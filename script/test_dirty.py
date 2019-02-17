#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys 
import os
import datetime
import random
import db #引入存盘模块
import doc_name #引入全局文档名模块

_SUB_DOC_1 = "sub_doc_1" #子文档1
_SUB_DOC_2 = "sub_doc_2" #子文档2
_SUB_DOC_LIST = (_SUB_DOC_1, _SUB_DOC_2) #子文档列表，为了安全，不能操作顶层文档，只能操作子文档，支持动态增加子文档

testdoc = db.load_dat_sync(doc_name.TEST_SYNC_DOC, _SUB_DOC_LIST) #同步加载一个存盘数据
testdoc[_SUB_DOC_1]["abc"] = { #修改存盘对象的子文档1
    "key1":1,
    "key3":[1,2,3],
}

subdoc = testdoc[_SUB_DOC_2] #修改存盘对象的子文档2
subdoc["key3"] = [1,2,34,5]
subdoc["key2"] = {1:2,3:3}

def _load_dat_cb(state, doc_obj, arg1, arg2 , arg3): #回调函数
    print("on async load dat cb state=%d,docobj=%s,arg1=%d,arg2=%d, arg3=%d"%(state, doc_obj, arg1, arg2, arg3))
    db.save_all(1) #执行一次增量存盘

db.load_dat_async(doc_name.TEST_ASYNC_DOC, _SUB_DOC_LIST, _load_dat_cb, 1,2,3) #异步加载一个存盘数据，加载后回调

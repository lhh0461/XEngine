#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys 
import os
import datetime
import random
from doc_name import HELLO_WORLD_DOC 
import db

print("test.py start restart doc")

_SUB_DOC_ACT_INFO = "actinfo"
_SUB_DOC_JOIN_INFO = "joininfo"
_SUB_DOC_EXAM_INFO = "examinfo"
_SUB_DOC_LIST = (_SUB_DOC_ACT_INFO, _SUB_DOC_JOIN_INFO, _SUB_DOC_EXAM_INFO)

doc = db.load_dat_sync(HELLO_WORLD_DOC, _SUB_DOC_LIST)
doc["actinfo"]["abc"] = {
    "key1":1,
    "key2":{1:1,2:2},
    "key3":[1,2,3],
}
#del doc["actinfo"]["abc"]["key1"]
a = doc["actinfo"]["abc"]
a["key3"] = [1,2,34,5]
a["key2"] = {1:2,3:3}
db.save_all(1) 
'''

print("test.py start restart doc end")

print("doc.keys %s" %(doc.keys()))
print("doc=%s" ,doc)

def load_dat_cb(state, doc_obj, arg1, arg2 , arg3):
    print("on load dat cb state=%d,docobj=%s,arg1=%d,arg2=%d, arg3=%d"%(state, doc_obj, arg1, arg2, arg3))
    db.save_all(1) 

print("load dat async")
db.load_dat_async("12345", ("docs1","docs2","docs3"), load_dat_cb, 1,2,3)
db.load_dat_async(111111, ("docs11","docs2","docs3"), load_dat_cb, 1,2,3)
'''

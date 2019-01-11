import sys 
import os
import datetime
import json
import random
from doc_name import HELLO_WORLD_DOC 
import db

a = db.restore_doc(HELLO_WORLD_DOC)
aa = db.restore_doc(HELLO_WORLD_DOC)
print("a == aa" ,id(a) == id(aa))
print("a.dir" ,dir(a))
print("a.keys" ,a.keys())
print("a.values" ,a.values())
print("a init" ,a)
a["bb"] = 1
a[2] = 100
del a[1]
a[1] = 1
a["cc"] = {2:{3:3}}
c= a["cc"][2]
c[3] = 4
print("!!!!")
print("a after=",a)
#b = dirty.get_dirty_info(a)
#print("b=",b)
print("c type=%s, value=%s"%(type(c), c))
a["dd"] = [2,3,4]
d = a["dd"]
d[0] = [5]
d.append(6)
print("d type=%s, value=%s"%(type(d), d))
print("a after=",a)

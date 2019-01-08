import sys 
import os
import datetime
import json
import random

sys.path.append(os.path.join("build", "lib.linux-x86_64-2.6"))
sys.path.append(os.path.join("build", "lib.linux-x86_64-2.7"))

import dirty
a = dirty.load('orgact')
print(a.keys())
print(a)
b = a.pop(2)
print("b= !!!", b)
print("after a= !!!", a)
print(a.values())
print("test7 !!!")
c = a[1]
print("type c!!!", type(c))
print("c=!!!", c)
print("a=!!!", a)
b = a.popitem()
print("a.keys", a.keys())
print("b", b)
'''
a["2"] = 2
b = a["1"]
c = a["1"]
#print "b=%d" % b
print type(b)
print type(a)
print id(b) == id(c)
print "before ",a
c[3]=3
print "after ",a
a["3"] = 3
'''
print type(c)
#print(a.pop("1"))
#print(a.keys())

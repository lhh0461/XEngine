import sys 
import os
import datetime
import json
import random

sys.path.append(os.path.join("build", "lib.linux-x86_64-2.6"))
sys.path.append(os.path.join("build", "lib.linux-x86_64-2.7"))

import dirty
print(dir(dirty))
a = dirty.dirty_dict()
dirty.load(a,'orgact')
print(dir(a))
print(a)
#b={1:1,2:2}
#a["1"] = 1
c = a["orgact"]
#b = a.pop("orgact")
b = a.popitem()
print(a)
print(b)
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

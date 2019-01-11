import sys

a = {1:1,5:5,2:2,3:3,4:4, 6:{7:{8:8}}}
print a
b = a.pop(4)
print a
print b
c = [1,2,3,4]
c[-2] = 100
print c
d = a[6][7]
print "d=",d
print "a=",a
dd="1234"
ff="1234"
print "id=%d,id=%d,ref=%d,ref=%d" %(id(dd), id(ff), sys.getrefcount(dd), sys.getrefcount(ff))

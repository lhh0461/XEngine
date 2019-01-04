#! /usr/bin/env python
# -*- coding: utf-8 -*-
#from example import person_pb2
import person_pb2

# 为 all_person 填充数据
pers = person_pb2.all_person()
p1 = pers.Per.add()
p1.id = 1
p1.name = 'xieyanke'
p2 = pers.Per.add()
p2.id = 2
p2.name = 'pythoner'

# 对数据进行序列化
data = pers.SerializeToString()

# 对已经序列化的数据进行反序列化
target = person_pb2.all_person()
target.ParseFromString(data)
print(target.Per[1].name)  #  打印第一个 person name 的值进行反序列化验证
print(target.Per[0].name)  #  打印第一个 person name 的值进行反序列化验证

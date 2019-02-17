#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import codecs
import os

struct_id2name = {}
struct_name2id = {}

function_id2key = {}
function_key2id = {}

all_struct_list = {}
all_function_list = {}

field2id = {
    "int":1,
    "string":2,
    "struct":3,
    "float":4,
}

def parse_struct(struct_name, struct_data):
    if struct_name in all_struct_list:
        raise Exception("repeated struct name is unsupported, ", struct_name)

    if not isinstance(struct_data, list):
        raise Exception("struct data must be list", struct_name)

    field_list = []
    for field_data in struct_data: 
        if not isinstance(field_data, dict):
            raise Exception("field data must be dict", field_data)

        if "TYPE" not in field_data:
            raise Exception("field data must set type", field_data)

        if field_data["TYPE"] == "struct" and "STRUCT" not in field_data:
            raise Exception("field data must set type", field_data)

        data = {}
        data["name"] = field_data["NAME"]
        data["type"] = field2id[field_data["TYPE"]]
        data["array"] = field_data["ARRAY"]
        data["struct"] = field_data.get("STRUCT", 0)

        if "DEFAULT" in field_data:
            data["default"] = field_data["DEFAULT"]
        field_list.append(data)

    all_struct_list[struct_name] = field_list
    
def parse_args(arg_data):
    ret = {}
    if "STRUCT" in arg_data:
        ret["struct"] = arg_data["STRUCT"]
    else:
        ret["struct"] = 0
        if arg_data["TYPE"] == "STRUCT":
            raise Exception("struct arg must set \"STRUCT\" field")

    ret["type"] = field2id[arg_data["TYPE"]]
    ret["array"] = arg_data["ARRAY"]
    return ret

def parse_function(module_path, function_data):
    if "FUNCTION_NAME" in function_data:
        function_name = function_data["FUNCTION_NAME"]

        arg_info = []
        if "ARG_LIST" in function_data:
            arg_list = function_data["ARG_LIST"]
            for arg_data in arg_list:
                arg_info.append(parse_args(arg_data))

        c_imp = function_data["C_IMP"]
        deamon = function_data["DEAMON"]
        description = function_data.get("DESCRIPTION", "")
        function_key = module_path + ":" + function_name
        all_function_list[function_key] = {
            "module":module_path,
            "function":function_name,
            "c_imp" :c_imp,
            "deamon" :deamon,
            "arg_list" :arg_info,
            "name" :function_name,
            "description" :description,
        }

    else:
        raise Exception("must define function name")

def parse_module(module_data):
    module_path = ""
    if "MODULE_PATH" in module_data:
       module_path = module_data["MODULE_PATH"]
    else:
        raise Exception("must define module path")

    if "FUNCTION_LIST" in module_data:
        function_list = module_data["FUNCTION_LIST"]
        for function_data in function_list:
            parse_function(module_path, function_data)
    else:
        raise Exception("must define function list")

    if "STRUCT_LIST" in module_data:
        struct_list = module_data["STRUCT_LIST"]
        for name,struct_data in struct_list.items():
            parse_struct(name, struct_data)

def parse():
    files = os.listdir("rpc")
    for f in files:
        if os.path.splitext(f)[-1] == ".json":
            with open("rpc/" + f, 'r') as f:
                rpcdata = json.load(f)
                if "MODULE_LIST" in rpcdata:
                    module_list = rpcdata["MODULE_LIST"]
                    for module_data in module_list:
                        parse_module(module_data)


def sort_struct():
    global struct_id2name
    global struct_name2id
    struct_id = 1
    sorted_list = all_struct_list.keys()
    sorted_list.sort()
    for struct_name in sorted_list:
        struct_id2name[struct_id] = struct_name
        struct_name2id[struct_name] = struct_id
        struct_id += 1
         
def sort_function():
    global function_key2id
    global function_id2key
    function_id = 1
    sorted_list = all_function_list.keys()
    sorted_list.sort()
    for function_key in sorted_list:
        function_id2key[function_id] = function_key
        function_key2id[function_key] = function_id
        function_id += 1

def write_file():
    global struct_id2name
    global struct_name2id
    global function_id2key
    with open("rpc/rpc.cfg", 'w') as f:
        f.write('struct_table_num:{num}\n'.format(num=len(all_struct_list))) 
        sorted_id_list = struct_id2name.keys()
        for struct_id in sorted_id_list:
            struct_name = struct_id2name[struct_id]
            struct_data = all_struct_list[struct_name]
            f.write('struct_id:{struct_id},field_num:{field_num},struct_name={struct_name}\n'.
                    format(struct_id=struct_id, field_num=len(struct_data), struct_name=struct_name)) 
            for field_data in struct_data:
                struct_id = struct_name2id.get(field_data["struct"],0) 
                f.write('field_type:{field_type},struct_id:{struct_id},array:{array},field_name={field_name}\n'.
                    format(field_type=field_data["type"], struct_id=struct_id, array=field_data["array"], field_name=field_data["name"])) 
            
        f.write('function_table_num:{num}\n'.format(num=len(all_function_list))) 
        sorted_id_list = function_id2key.keys()
        for function_id in sorted_id_list:
            function_key = function_id2key[function_id]
            function_data = all_function_list[function_key]
            f.write('function_id:{function_id},arg_num:{arg_num},module={module},function={function},c_imp={c_imp},deamon={deamon}\n'.
                    format(function_id=function_id,arg_num=len(function_data["arg_list"]),module=function_data["module"],function=function_data["function"],
                        c_imp=function_data["c_imp"],deamon=function_data["deamon"])) 
            for arg_data in function_data["arg_list"]:
                struct_id = struct_name2id.get(arg_data["struct"],0) 
                f.write('arg_type:{arg_type},struct_id:{struct_id},array:{array}\n'.
                    format(arg_type=arg_data["type"], struct_id=struct_id, array=arg_data["array"]))

    with codecs.open("script/rpc_id.py", "w", "utf-8") as f:
        print >>f, u'# -*- coding: utf-8 -*-'
        print >>f, u'# 此文件由脚本生成，请勿修改！'
        print >>f, u''
        print >>f, u'# 以下是协议ID'
        sorted_id_list = function_id2key.keys()
        for function_id in sorted_id_list:
            function_key = function_id2key[function_id]
            function_data = all_function_list[function_key]
            print >>f , u'{function_name} = {function_id} #{description}'.format(function_name=function_data["function"].upper(), function_id=function_id, description=function_data["description"])
            

                
if __name__ == '__main__':
    parse()
    sort_struct()
    sort_function()
    #check_struct()
    write_file()
    print"parse rpc ok"

#    print("show function list\n",all_function_list)
#    print("show struct list\n",all_struct_list)
#    print("show struct name2id\n",struct_name2id)
#    print("show struct id2name\n",struct_id2name)


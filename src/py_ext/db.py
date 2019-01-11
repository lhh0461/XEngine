#!/usr/bin/python
import sys 
import os

sys.path.append(os.path.join("build", "lib.linux-x86_64-2.6"))
sys.path.append(os.path.join("build", "lib.linux-x86_64-2.7"))

import dirty

loaded_doc_dict = {}

def restore_doc(doc_name):
    global doc_list
    if loaded_doc_dict.has_key(doc_name):
        return loaded_doc_dict[doc_name]
    doc = dirty.restore(doc_name)
    loaded_doc_dict[doc_name] = doc
    return doc

#
#def restore_doc(doc_name):
#    global doc_list
#    if loaded_doc_dict.has_key(doc_name):
#        return loaded_doc_dict[doc_name]
#    doc = dirty.restore(doc_name)
#    loaded_doc_dict[doc_name] = doc
#    return doc



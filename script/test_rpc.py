#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rpc_id
import rpc

rpc.send(uid, rpc_id.RPC_CLIENT_HELLO_WORLD, {"uid":111, "msg":"test"}, 1)

def rpc_server_hello_world(uid, hello_req, num)
       

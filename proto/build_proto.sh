protoc --python_out=./output proto_py/test.proto
protoc --cpp_out=./output proto_py/test.proto
cd tool && lua ./trans_rpc_cfg.lua  ../rpc_decl/ ../output/

import sys
sys.path.append("proto/output/proto_py/")
import test_pb2  

def rpc_server_test_hello(buff):
    pbHelloRequest = test_pb2.HelloRequest()  
    pbHelloRequest.ParseFromString(buff)
    print(pbHelloRequest.greeting)

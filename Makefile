PROJECT=server
CC=gcc
CFLAGS=-W

CXX=g++
CXXFLAGS=-W -std=c++11

LIB_DIR:=lib
SRC_DIR:=src
INC_DIR:=include
THIRD_PART_DIR:=3rd
LIBEVENT_DIR:=$(THIRD_PART_DIR)/libevent
PROTO_DIR:=$(THIRD_PART_DIR)/protobuf

INC_FILES:=-I$(INC_DIR) \
	-I$(LIB_DIR)/include \
	-I$(LIBEVENT_DIR)/include\
	-I$(PROTO_DIR)/include\
	-I/usr/include/python2.7 

C_SRC_FILES:=$(foreach v, $(SRC_DIR), $(wildcard $(v)/*.c))
CXX_SRC_FILES:=$(foreach v, $(SRC_DIR), $(wildcard $(v)/*.cpp))

C_OBJ_FILES:=$(subst .c,.o,$(C_SRC_FILES))
CXX_OBJ_FILES:=$(subst .cpp,.o,$(CXX_SRC_FILES))

LIBS=-L$(LIB_DIR)/lib \
	-L$(LIBEVENT_DIR)/lib \
	-L$(PROTO_DIR)/lib

STATIC_LIBS= -levent  \
	-lpython2.7 \
	-lprotobuf

all:$(C_OBJ_FILES) $(CXX_OBJ_FILES)
	$(CC) $(CFLAGS) $(C_OBJ_FILES) $(CXX_OBJ_FILES) -o $(PROJECT) $(INC_FILES) $(LIBS) $(STATIC_LIBS)

$(SRC_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -c -o $@ $(INC_FILES) $(LIBS) $(STATIC_LIBS)

$(SRC_DIR)/%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@ $(INC_FILES) $(LIBS) $(STATIC_LIBS)

.PHONY:clean
clean:
	-rm $(PROJECT)
	-rm -rf $(SRC_DIR)/*.o

PROJECT=server
CC=gcc
CFLAGS=-W -g

CXX=g++
CXXFLAGS=-W -std=c++11 -g
PKG_FLAG=`pkg-config --libs --cflags glib-2.0`

LIB_DIR:=lib
SRC_DIR:=src
INC_DIR:=include
THIRD_PART_DIR:=3rd
LIBEVENT_DIR:=$(THIRD_PART_DIR)/libevent
PROTO_DIR:=$(THIRD_PART_DIR)/protobuf
MONGOC_DIR:=$(THIRD_PART_DIR)/mongo-c-driver
BSON_DIR:=$(THIRD_PART_DIR)/mongo-c-driver

INC_FILES:=-I$(INC_DIR) \
	-I$(LIB_DIR)/include \
	-I$(LIBEVENT_DIR)/include\
	-I$(PROTO_DIR)/include\
	-I$(MONGOC_DIR)/include/libmongoc-1.0\
	-I$(BSON_DIR)/include/libbson-1.0\
	-I/usr/include/python3.5

C_SRC_FILES:=$(foreach v, $(SRC_DIR), $(wildcard $(v)/*.c))
CXX_SRC_FILES:=$(foreach v, $(SRC_DIR), $(wildcard $(v)/*.cpp))

C_OBJ_FILES:=$(subst .c,.o,$(C_SRC_FILES))
CXX_OBJ_FILES:=$(subst .cpp,.o,$(CXX_SRC_FILES))

LIBS=-L$(LIB_DIR)/lib \
	-L$(LIBEVENT_DIR)/lib \
	-L$(PROTO_DIR)/lib \
	-L$(MONGOC_DIR)/lib

STATIC_LIBS= -levent  \
	-lpython3.5m \
	-lprotobuf \
	-lmongoc-1.0 \
	-lbson-1.0

all:$(C_OBJ_FILES) $(CXX_OBJ_FILES)
#$(CC) $(CFLAGS) $(C_OBJ_FILES) $(CXX_OBJ_FILES) -o $(PROJECT) $(INC_FILES) $(LIBS) $(STATIC_LIBS) $(PKG_FLAG)
	$(CC) $(CFLAGS) $(C_OBJ_FILES) $(CXX_OBJ_FILES) -o $(PROJECT) $(LIBS) $(STATIC_LIBS) $(PKG_FLAG)

$(SRC_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -c -o $@ $(INC_FILES) $(LIBS) $(STATIC_LIBS) $(PKG_FLAG)

$(SRC_DIR)/%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@ $(INC_FILES) $(LIBS) $(STATIC_LIBS)

.PHONY:clean
clean:
	-rm $(PROJECT)
	-rm -rf $(SRC_DIR)/*.o

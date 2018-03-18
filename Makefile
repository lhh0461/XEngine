CC=gcc
CFLAGS=-W

LIB_DIR:=./lib
OBJ_DIR:=./obj
SRC_DIR:=./src
INC_DIR:=./include

INC_FILES:=-I$(INC_DIR) \
	-I$(LIB_DIR)/include

SRC_FILES:=$(foreach v, $(SRC_DIR), $(wildcard $(v)/*.c))

LIBS=-L$(LIB_DIR)/lib

STATIC_LIBS=-levent

all: $(SRC_FILES)
	$(CC) $(CFLAGS) -o main $(SRC_FILES) $(INC_FILES) $(LIBS) $(STATIC_LIBS)

.PHONY:clean
clean:
	-rm main obj/*

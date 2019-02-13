#!/bin/sh

prefix=/home/lhh//workspace/engine/3rd/jemalloc
exec_prefix=/home/lhh//workspace/engine/3rd/jemalloc
libdir=${exec_prefix}/lib

LD_PRELOAD=${libdir}/libjemalloc.so.2
export LD_PRELOAD
exec "$@"

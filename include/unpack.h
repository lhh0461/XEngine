#ifndef __UNPACK_H__
#define __UNPACK_H__

#include <msgpack.h>

typedef struct msgpack_unpacker_s {
    char *buf;
    size_t len;
    size_t offset;
    msgpack_unpacked pack;
} msgpack_unpacker_t;

static inline msgpack_unpack_return msgpck_unpacker_next_pack(msgpack_unpacker_t *unpacker)
{
    return msgpack_unpack_next(&unpacker->pack, unpacker->buf, unpacker->len, &unpacker->offset);
}

static inline void construct_msgpack_unpacker(msgpack_unpacker_t *unpacker, char *data, int len)
{
    msgpack_unpacked_init(&unpacker->pack);
    unpacker->buf = data;
    unpacker->len = len;
    unpacker->offset = 0;
}

static inline void destroy_msgpack_unpackert(msgpack_unpacker_t *unpacker)
{
    msgpack_unpacked_destroy(&unpacker->pack);
}

#define GET_UNPACKER_PACK_TYPE(unpacker) ((unpacker)->pack.data.type)

#endif //__UNPACK_H__


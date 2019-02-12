#include <stdio.h>
#include <assert.h>
#include <msgpack.h>

typedef struct some_struct {
    uint32_t a;
    uint32_t b;
} some_struct;

static char *pack(const some_struct *s, int num, int *size);
static some_struct *unpack(const void *ptr, int size, int *num);

/* Fixtures */
some_struct ary[] = {
    { 1234, 5678},
    { 4321, 8765},
    { 2143, 6587}
};

int main(void) {
    /** PACK */
    int size;
    char *buf = pack(ary, sizeof(ary)/sizeof(ary[0]), &size);
    printf("pack %zd struct(s): %d byte(s)\n", sizeof(ary)/sizeof(ary[0]), size);

    /** UNPACK */
    int num = -1;
    some_struct *s = unpack(buf, size, &num);
    printf("unpack: %d struct(s)\n", num);

    /** CHECK */
    assert(s);
    assert(num == (int) sizeof(ary)/sizeof(ary[0]));
    for (int i = 0; i < num; i++) {
        printf("s[i].a =%d. ary[i]=%d\n", s[i].a, ary[i].a);
        assert(s[i].a == ary[i].a);
        assert(s[i].b == ary[i].b);
        //assert(s[i].c == ary[i].c);
    }
    printf("check ok. Exiting...\n");

    free(buf);
    free(s);

    return 0;
}

/* Pack an input array made of `num` structs into a msgpack archive. The archive
 *    is composed of a single, root object (= array). In other words, the whole data
 *       is wrapped into a top-level array.
 *          In consequence unpacking the structure is pretty simple and does NOT require
 *             to use the general purpose `msgpack_unpacker` streaming deserializer.
 *                Another solution consists in packing multiple objects: a1, b1, c1, a2, ...
 *                   where ai, bi, ci refer to the fields of the i-th structure. By doing so the
 *                      unpacking logic requires the `msgpack_unpacker` streaming deserializer. */
static char *pack(const some_struct *s, int num, int *size) {
    assert(num > 0);
    char *buf = NULL;
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer pck;
    msgpack_packer_init(&pck, &sbuf, msgpack_sbuffer_write);
    /* The array will store `num` contiguous blocks made of a, b, c attributes */
    msgpack_pack_array(&pck, 2 * num);
    for (int i = 0; i < num; ++i) {
        msgpack_pack_uint32(&pck, s[i].a);
        msgpack_pack_uint32(&pck, s[i].b);
        //msgpack_pack_float(&pck, s[i].c);
    }
    *size = sbuf.size;
    buf = malloc(sbuf.size);
    memcpy(buf, sbuf.data, sbuf.size);
    msgpack_sbuffer_destroy(&sbuf);
    return buf;
}

static some_struct *unpack(const void *ptr, int size, int *num) {
    some_struct *s = NULL;
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    if (msgpack_unpack_next(&msg, ptr, size, NULL)) {
        msgpack_object root = msg.data;
        if (root.type == MSGPACK_OBJECT_ARRAY) {
            assert(root.via.array.size % 3 == 0);
            *num = root.via.array.size / 2;
            printf("unpack size=%d\n", root.via.array.size);
            s = malloc(root.via.array.size*sizeof(*s));
            for (int i = 0, j = 0; i < root.via.array.size; i += 2, j++) {
                s[j].a = root.via.array.ptr[i].via.u64;
                s[j].b = root.via.array.ptr[i + 1].via.u64;
                //s[j].c = root.via.array.ptr[i + 2].via.dec;
            }
        }
    }
    msgpack_unpacked_destroy(&msg);
    return s;
}

#ifndef __BUFFER_H__
#define __BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

//模仿evbuffer实现的buffer模块，把内存块用链表连起来，逻辑空间上是连续的

//内存块
typedef struct block_s {
    struct block_s *next;           //下一个块
    //struct buffer_s *buffer;           //下一个块
    char *buf;             //内存块开始地址
    char *end;             //内存块结束地址
    char *tail;            //内存块未读取数据尾地址
    char *head;            //内存块未读取数据头地址
    unsigned block_size;        //本内存块分配大小
} block_t;

typedef struct buffer_s {
    block_t *head;                  //头内存块
    block_t *tail;                  //尾内存块（每次从尾块插入数据）
    unsigned alloc_size;     //所有块分配的总内存数
    unsigned data_size;         //当前还有没处理数据数
    unsigned block_num;         //分配的块数
} buffer_t;

//返回堆分配的buffer，不用再调初始化
buffer_t *buffer_new(void);
void buffer_free(buffer_t *buf);

//初始化buffer，分配内存块
int buffer_init(buffer_t *buf);
//只清除buffer里的内存块，不释放buf
void buffer_release(buffer_t *buf);
void buffer_reset(buffer_t *buf);

int buffer_get_data_size(const buffer_t *buf);
int buffer_get_alloc_size(const buffer_t *buf);

//这个函数存储的数据不是连续的
int buffer_push(buffer_t *buf, const char *data, unsigned len);
int buffer_remove(buffer_t *buf, char *data_out, unsigned data_len);

char *buffer_pullup(buffer_t *buf);
int buffer_drain(buffer_t *buf, unsigned len);
int buffer_add_block(buffer_t *buffer, unsigned size);

#define BUFFER_BLOCK_DATA_LEN(blk) ((blk)->tail - (blk)->head)
#define BUFFER_BLOCK_FREE_LEN(blk) ((blk)->end - (blk)->tail)
#define BUFFER_BLOCK_USED_LEN(blk) ((blk)->head - (blk)->buf)
#define BUFFER_DATA_ADVANCE(buf, len)   \
    do {                                \
        (buf)->tail->tail += (len);     \
        (buf)->data_size += (len);      \
    } while (0);

//这个函数返回连续的内存空间
inline static void *buffer_malloc(buffer_t *buf, unsigned size)
{
    if (!buf) return NULL;
    if (size <= 0) return NULL;

    block_t *block = buf->tail;
    if (!block) return NULL;

    if (BUFFER_BLOCK_FREE_LEN(buf->tail) < size) {
        if (!buffer_add_block(buf, size)) {
            return NULL;
        }
        INTERNAL_ASSERT(BUFFER_BLOCK_DATA_LEN(buf->tail) == 0);
    }

    BUFFER_DATA_ADVANCE(buf, size);
    return buf->tail->tail - size;
}

#ifdef __cplusplus
}
#endif

#endif //__BUFFER_H__


#ifndef __BUFFER_H__
#define __BUFFER_H__

//模仿evbuffer实现的buffer模块，把内存块用链表连起来，逻辑空间上是连续的

//内存块
typedef struct block_s {
    struct block_s *next;           //下一个块
    char *buf;             //内存块开始地址
    char *end;             //内存块结束地址
    char *tail;            //内存块未读取数据尾地址
    char *head;            //内存块未读取数据头地址
    unsigned block_size;        //本内存块分配大小
} block_t;

typedef struct buffer_s {
    block_t *head;                  //头内存块
    block_t *tail;                  //尾内存块（每次从尾块插入数据）
    block_t *cur;                   //当前正在处理的内存块
    unsigned allocate_size;     //所有块分配的总内存数
    unsigned data_size;         //当前还有没处理数据数
    unsigned block_num;         //分配的块数
} buffer_t;

buffer_t *buffer_new(void);
void buffer_free(buffer_t *buf);
void buffer_get_data_size(buffer_t *buf);
void buffer_get_alloc_size(buffer_t *buf);

void buffer_push(buffer_t *buf, const char *data, unsigned len);
void buffer_remove(buffer_t *buf, char *data_out, unsigned data_len);
char *buffer_pullup(buffer_t *buf, unsigned len);
void buffer_drain(buffer_t *buf, unsigned len);

#define BUFFER_BLOCK_CAP(blk) ((blk)->end - (blk)->tail)
#define BUFFER_BLOCK_DATA_LEN(blk) ((blk)->tail - (blk)->head)

static inline void *buffer_malloc(buffer_t *buf, unsigned size)
{
    if (buf) {
        if (buf->tail && BUFFER_BLOCK_CAP(buf->tail) < size) {
            if (!buffer_add_block(buf, size)) {
                return NULL;
            }
            assert(BUFFER_BLOCK_DATA_LEN(buf->tail) == 0);
        }
        buf->tail->tail += size;
        buf->data_size += size;
        return buf->tail->tail - size;
    }
}

#endif //__BUFFER_H__

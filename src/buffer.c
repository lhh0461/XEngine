#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"

//空闲列表
//1K,2K,4K,8K,16K
#define BLOCK_FACTOR (4)
#define FREE_BLOCK_NUM (10)
#define FREE_BLOCK_BASE (1024)
#define FREE_BLOCK_MAX (FREE_BLOCK_BASE << 4)
static block_t *g_free_list[5] = {NULL, NULL, NULL, NULL, NULL};
static unsigned int g_free_list_num[5] = {0, 0, 0, 0, 0};

unsigned get_allocate_size(unsigned need_size)
{
    unsigned alloc_size;
    if (need_size > FREE_BLOCK_MAX) {
        alloc_size = BLOCK_FACTOR * ((need_size + BLOCK_FACTOR - 1) / BLOCK_FACTOR);
        return alloc_size;
    }

    //小于16K，返回固定大小的block
    alloc_size = FREE_BLOCK_BASE;
    while (alloc_size < need_size) alloc_size <<= 1;

    return alloc_size;
}

static int get_free_list_idx(unsigned size)
{
    if (size <= FREE_BLOCK_BASE) return 0;
    if (size <= FREE_BLOCK_BASE << 1) return 1;
    if (size <= FREE_BLOCK_BASE << 2) return 2;
    if (size <= FREE_BLOCK_BASE << 3) return 3;
    if (size <= FREE_BLOCK_BASE << 4) return 4;
    return -1;
}

static block_t *pop_free_block(unsigned size)
{
    block_t *block = NULL;
    if (size > FREE_BLOCK_MAX) {
        return NULL;
    }

    int idx = get_free_list_idx(size);
    if (idx == -1) return NULL;

    block = g_free_list[idx];
    if (block == NULL) {
        return NULL;
    }

    block = g_free_list[idx];
    g_free_list[idx] = block->next;
    g_free_list_num[idx]--;

    return block;
}

static int push_free_block(block_t *block)
{
    if (block == NULL) return 0;

    if (block->block_size > FREE_BLOCK_MAX) {
        return 0;
    }

    int idx = get_free_list_idx(block->block_size);
    if (idx == -1) return 0;

    if (g_free_list_num[idx] >= FREE_BLOCK_NUM) {
        return 0;
    }

    block->next = g_free_list[idx];
    g_free_list[idx] = block;
    g_free_list_num[idx]++;
    return 1;
}

inline static void block_buf_init(block_t *block)
{
    block->head = block->tail = block->buf;
}

static block_t *new_block(unsigned size)
{
    block_t * block = NULL;

    //尝试从free_list里取 
    if (size < FREE_BLOCK_MAX) { 
        if ((block = pop_free_block(size)) != NULL) {
            return block;
        }
    }
    
    unsigned alloc_size = get_allocate_size(size);
    block = (block_t *)malloc(sizeof(block_t) + size);
    if (!block) {
        return NULL;
    }
    
    block->buf = (char *)(block + 1);
    block->block_size = alloc_size;
    block->end = block->buf + block->block_size;
    block->next = NULL;
    block_buf_init(block);
    return block;
}

static void del_block(block_t *block)
{
    if (block == NULL) return;
    if (push_free_block(block)) {
        block_buf_init(block);
        return;
    }

    free(block);
}

int buffer_add_block(buffer_t *buffer, unsigned size)
{
    block_t *block = new_block(size);
    if (!block) return 0;

    if (buffer->tail == NULL) {
        buffer->head = buffer->tail = block;
    } else {
        buffer->tail->next = block;
        buffer->tail = block;
    }   

    buffer->block_num++;
    buffer->alloc_size += block->block_size;
    return 1;
}

buffer_t *buffer_new(void)
{
    buffer_t *buffer = (buffer_t *)calloc(1, sizeof(buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    buffer->data_size = 0;
    if (!buffer_add_block(buffer, FREE_BLOCK_BASE)) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

void buffer_free(buffer_t *buf)
{
    buffer_release(buf);
    free(buf);
}

void buffer_release(buffer_t *buf)
{
    block_t *block, *tmp;

    for (block = buf->head; block && (tmp = block->next, 1); block = tmp) {
        del_block(block);
    }

    buf->alloc_size = 0;
    buf->data_size = 0;
    buf->block_num = 0;
    buf->head = NULL;
    buf->tail = NULL;
}

int buffer_init(buffer_t *buf)
{
    if (!buf) return 0;
    //不能重复初始化
    if (buf->head != NULL) return 0;

    buf->data_size = 0;
    if (!buffer_add_block(buf, FREE_BLOCK_BASE)) {
        return 0;
    }
    return 1;
}

void buffer_reset(buffer_t *buf)
{
    if (buf->block_num > 1) {
        buffer_release(buf);
        buffer_init(buf);
    } else {
        block_buf_init(buf->head);
        buf->data_size = 0;
    }   
    INTERNAL_ASSERT(buf->head == buf->tail);
}

int buffer_push(buffer_t *buffer, const char *data, unsigned len)
{
    const char *data_ptr = data;
    unsigned howmuch = 0;

    if (!buffer) return -1;

    block_t *block = buffer->tail;
    if (!block) return -1;

    if (!data_ptr) return 0;
    if (len <= 0) return 0;

    unsigned free_len = BUFFER_BLOCK_FREE_LEN(block);
    if (free_len > 0) {
        unsigned push_len = len > free_len ? free_len : len;
        memcpy(block->tail, data_ptr, push_len);
        len -= push_len;
        data_ptr += push_len;
        BUFFER_DATA_ADVANCE(buffer, push_len);
        howmuch += push_len;
    }

    if (len > 0) {
        if (!buffer_add_block(buffer, len)) {
            return howmuch;
        }
        INTERNAL_ASSERT(BUFFER_BLOCK_DATA_LEN(buffer->tail) == 0);
        INTERNAL_ASSERT(BUFFER_BLOCK_FREE_LEN(buffer->tail) >= len);
        block_t *block = buffer->tail;
        memcpy(block->tail, data_ptr, len);
        BUFFER_DATA_ADVANCE(buffer, len)
        howmuch += len;
    }
    return howmuch;
}

int buffer_remove(buffer_t *buf, char *data_out, unsigned data_len)
{
    char *data_ptr = data_out;
    unsigned howmuch = 0;
    block_t *block = buf->head;
    block_t *tmp;

    if (data_len > buf->data_size) {
        data_len = buf->data_size;
    }

    while (data_len > 0 && block) {
        unsigned paylen = BUFFER_BLOCK_DATA_LEN(block);
        unsigned remove_len =  data_len > paylen ? paylen : data_len;
        if (data_ptr != NULL) {
            memcpy(data_ptr, block->head, remove_len);
            data_ptr += remove_len;
        }
        data_len -= remove_len;

        block->head += remove_len;
        tmp = block;
        block = block->next;
        buf->data_size -= remove_len;
        howmuch += remove_len;

        //这个内存块内存已经用完，并且数据已经全部读取完毕，释放内存块
        if (BUFFER_BLOCK_FREE_LEN(tmp) == 0 && BUFFER_BLOCK_USED_LEN(tmp) == tmp->block_size) {
            buf->head = tmp->next;
            buf->alloc_size -= tmp->block_size;
            buf->block_num -= 1;
            del_block(tmp);
        }
    }

    return howmuch;
}

int buffer_drain(buffer_t *buf, unsigned len)
{
    return buffer_remove(buf, NULL, len);
}

char *buffer_pullup(buffer_t *buf)
{
    if (buf == NULL) return NULL;
    if (buf->block_num <= 0) return NULL;
    if (buf->block_num == 1) return buf->head->head;

    block_t *new_blk = new_block(buf->data_size);

    int offset = 0;
    block_t *block = buf->head;
    while (block != NULL) {
        int len = BUFFER_BLOCK_DATA_LEN(block);
        if (len > 0) {
            memcpy(new_blk->buf + offset, block->head, len);
            offset += len;
        }

        block_t *tmp = block;
        block = block->next;
        del_block(tmp);
    }

    new_blk->tail = new_blk->head + offset;

    buf->head = buf->tail = new_blk;
    buf->alloc_size = new_blk->block_size;
    buf->block_num = 1;

    return new_blk->head;
}

int buffer_get_data_size(const buffer_t *buf)
{
    if (buf == NULL) return 0;
    return buf->data_size;
}

int buffer_get_alloc_size(const buffer_t *buf)
{
    if (buf == NULL) return 0;
    return buf->alloc_size;
}

/*
int main()
{
    buffer_t *buffer = buffer_new();
    char data[4888] = {0};
    memcpy(data, "hello world\n", 13);
    buffer_push(buffer, data, 4888);
    buffer_push(buffer, data, 4888);
    buffer_push(buffer, data, 4888);
    buffer_push(buffer, data, 4888);
    buffer_push(buffer, data, 4888);
    printf("test push data_size=%d, alloc_size=%d, block_num=%d\n", buffer->data_size, buffer->alloc_size, buffer->block_num);

    block_t *block = buffer->head;
    while (block != NULL) {
        int len = BUFFER_BLOCK_DATA_LEN(block);
        printf("block_size=%d,data_len=%ld,free_len=%ld\n", block->block_size, BUFFER_BLOCK_DATA_LEN(block), BUFFER_BLOCK_FREE_LEN(block));
        block = block->next;
    }
    
    char data2[4888];
    memset(data2, 0, 4888);
    buffer_remove(buffer, data2, 4888);
    buffer_remove(buffer, data2, 4888);
    buffer_remove(buffer, data2, 4888);
    buffer_remove(buffer, data2, 4888);
    buffer_remove(buffer, data2, 4888);

    for (int i = 0; i < 5; i++) {
        printf("g_free_list_num=%d, i=%d\n", g_free_list_num[i], i);
    }

    char *ptr = buffer_pullup(buffer);
    printf("ptr=%s\n",ptr);
    //buffer_remove(buffer, data2, 4888);
    printf("data_size=%d, alloc_size=%d, block_num=%d\n", buffer->data_size, buffer->alloc_size, buffer->block_num);
    //buffer_push(buffer, data, 4888);
    block = buffer->head;
    while (block != NULL) {
        int len = BUFFER_BLOCK_DATA_LEN(block);
        printf("block_size=%d,data_len=%ld,free_len=%ld,used_len=%ld\n", block->block_size, BUFFER_BLOCK_DATA_LEN(block), BUFFER_BLOCK_FREE_LEN(block), BUFFER_BLOCK_USED_LEN(block));
        block = block->next;
    }

    for (int i = 0; i < 5; i++) {
        printf("g_free_list_num=%d, i=%d\n", g_free_list_num[i], i);
    }
}
*/

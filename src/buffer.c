#include "buffer.h"

//空闲列表
//1K,2K,4K,8K,16K
#define BLOCK_FACTOR (4)
#define FREE_BLOCK_BASE (1024)
#define FREE_BLOCK_MAX (FREE_BLOCK_BASE << 4)
static block_t *g_free_list[5] = {NULL, NULL, NULL, NULL, NULL};

unsigned get_allocate_size(unsigned need_size)
{
    unsigned alloc_size;
    if (need_size > FREE_BLOCK_MAX) {
        alloc_size = BLOCK_FACTOR * ((need_size + BLOCK_FACTOR - 1) / BLOCK_FACTOR);
        return alloc_size;
    }

    //小于16K，返回固定大小的block
    alloc_size = FREE_LIST_MIN;
    while (alloc_size < need_size) {
        alloc_size <<= 1;
    }

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

    int idx = get_free_list(size);
    if (idx == -1) return NULL;

    block = g_free_list[idx];
    if (block == NULL) {
        return NULL;
    }

    block = g_free_list[idx];
    g_free_list[idx] = block->next;

    return block;
}

int push_free_block(block_t *block)
{
    if (block == NULL) return 0;

    if (block->size > FREE_LIST_MAX) {
        return 0;
    }

    int idx = get_free_list_idx(block->size);
    if (idx == -1) return 0;

    block->next = g_free_list[idx];
    g_free_list[idx] = block;
}

inline static void blk_buf_init(fs_mbuf_blk_t *blk)
{
    blk->head = blk->tail = blk->buf;
}

static block_t *new_block(unsigned size)
{
    block_t * block = NULL;

    //尝试从free_list里取 
    if (size < FREE_LIST_CACHE_MAX) { 
        if ((block = pop_free_block(size)) != NULL) {
            return block;
        }
    }
    
    unsigned alloc_size = get_allocate_size(size, NULL);
    block = (block_t *)malloc(sizeof(block_t) + size);
    if (!block) {
        return NULL;
    }
    
    block->buf = block + 1;
    block->size = alloc_size;
    block->end = block->buf + block->size;
    block->next = NULL;
    blk_buf_init(block);
    return block;
}

int buffer_add_block(buffer_t *buffer, unsigned size)
{
    block_t *block = new_block(size);
    if (!block) return 0;

    if (buffer->tail == NULL) {
        buffer->head = buffer->tail = blk;
    } else {
        buffer->tail->next = blk;
        buffer->tail = blk;
    }   

    buffer->blk_count++;
    buffer->alloc_size += blk->size;
    return 1;
}

buffer_t *buffer_new(void)
{
    buffer_t *buffer = (buffer_t *)calloc(1, sizeof(buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    buffer->data_size = 0;
    buffer_add_block(buffer, FREE_BLOCK_BASE);
    buffer->cur = buffer->head;
    return buffer;
}

void buffer_push(buffer_t *buf, const char *data, unsigned len)
{
    void * buf = buffer_malloc(buf, size);
    if (buf) {
        memcpy(buf, data, len);
    }
}

int buffer_remove(buffer_t *buf, char *data_out, unsigned data_len)
{
    char *data_ptr = data_out;
    unsigned remove_len = 0;
    block_t *block = buf->cur;

    if (data_len > buf->data_size) {
        data_len = buf->data_size;
    }

    while (data_len > 0 && block) {
        unsigned paylen = block->tail - block->head;

        if (data_ptr != NULL) {
            memcpy(data_ptr, block->head, paylen);
            data_ptr += paylen;
        }
        data_len -= paylen;

        block->head += paylen;
        block = block->next;
        buf->data_size -= size;
        buf->cur = block;
        remove_len += paylen;
    }

    return remove_len;
}

int buffer_drain(buffer_t *buf, unsigned len)
{
    return buffer_remove(buf, NULL, len);
}

char *buffer_pullup(buffer_t *buf, unsigned len)
{
    
}

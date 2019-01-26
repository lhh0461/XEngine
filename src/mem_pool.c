#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "mem_pool.h"
#include "log.h"

static int malloc_memory_size = 0;

static inline int _free_node_from_block(mem_block_t *block, void *node)
{
    int ent;

    if (!block) return 0;

    if (!block->buff) return 0;

    ent = ((char *)node - (char *)block->buff) / block->ent_size;

    if (ent < 0 || ent > block->ent_num) {
        return 0;
    }

    if (block->nodes[ent] != ALLOCATED) {
        log_error("unexpected free node not allocated!");
        return 1;
    }

    block->nodes[block->tail] = ent;   
    block->nodes[ent] = EOM;
    block->tail = ent;
    block->used_num--;
    return 1;
}

static inline void *_malloc_node_from_block(mem_block_t *block)
{
    if (block == NULL) return NULL;

    if (block->head == block->tail) {
        return NULL;
    }

    int ent = block->head;

    block->head = block->nodes[ent];
    block->nodes[ent] = ALLOCATED;
    block->used_num++;

    int offset = block->ent_size * ent;

    return (char *)block->buff + offset;
}

static inline mem_block_t *create_memory_block(int ent_size, int ent_num)
{
    mem_block_t *block;

    block = (mem_block_t *)malloc(sizeof(mem_block_t));
    if (block == NULL) {
        log_error("malloc memory block strcut fail.");
        return NULL;
    }

    block->ent_size = ent_size;
    block->ent_num = ent_num;
    block->used_num = 0;

    // ent_num+1: 多分配一个空间,使得末指针永远指向可使用的内存
    // 当head和tail指向同一块空间时,说明其他的ent_num个空间已分配完.
    block->buff = (char *)malloc(ent_size * (ent_num + 1));
    if (block->buff == NULL) {
        free(block);
        log_error("malloc memory block buf fail.");
        return NULL;
    }

    block->nodes = (int *)malloc(sizeof(int) * (ent_num + 1));
    if (block->nodes == NULL) {
        free(block->buff);
        free(block);
        log_error("malloc memory block node fail.");
        return NULL;
    }

    for (int i = 0; i <= ent_num; i++) {
        block->nodes[i] = i+1;
    }
    block->nodes[ent_num] = EOM; 
    block->head = 0;
    block->tail = ent_num;
    block->blk_size = ent_size * (ent_num + 1) + sizeof(int) * (ent_num + 1);

    malloc_memory_size += block->blk_size;

    log_info("create memory block: size=%d, ent=%d, total=%d", ent_size, ent_num, malloc_memory_size);
    return block;
}

//创建内存池
mem_pool_t *create_memory_pool(int ent_size, int ent_num)
{
    mem_pool_t *pool;
    mem_block_t *block;

    pool = (mem_pool_t *)malloc(sizeof(mem_pool_t));
    if (pool == NULL) {
        log_error("malloc memory pool sturct fail.");
        return NULL;
    }

    block = create_memory_block(ent_size, ent_num);
    if (block == NULL) {
        free(pool);
        log_error("create memory block fail.");
        return NULL;
    }

    block->next = NULL;

    pool->ent_num = ent_num;
    pool->ent_size = ent_size;
    pool->head = block;
    pool->tail = block;
    pool->total_size = block->blk_size;

    log_info("create memory pool success. ent_size:%d,ent_num:%d", ent_size, ent_num);
    return pool;
}

void *malloc_node(mem_pool_t *pool)
{
    mem_block_t *block;
    void *node;

    block = pool->head;
    while (block) {
        if ( (node = _malloc_node_from_block(block)) ) {
            return node;
        }
        block = block->next;
    }


    // 遍历无果，增加新块
    block = create_memory_block(pool->ent_size, pool->ent_num / 3 + 1);
    if (block == NULL) {
        log_error("create memory block on malloc node fail.");
        return NULL;
    }

    block->next = NULL;
    pool->tail->next = block;
    pool->tail = block;
    pool->total_size += block->blk_size;

    return _malloc_node_from_block(block);
}

int free_node(mem_pool_t *pool, void *node)
{
    mem_block_t *block;

    block = pool->head;
    while (block) {
        if (_free_node_from_block(block, node)) {
            return 1;
        }
        block = block->next;
    }
    return 0;
}

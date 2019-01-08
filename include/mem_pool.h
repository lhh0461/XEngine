#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

enum mem_st_e {
    ALLOCATED = 1,
    EOM = 2,
};

typedef struct mem_block_s {
	int head;        
	int tail;
	int *nodes;                //节点状态
	void * buff;
	int ent_num;               //内存块中实体个数
	int ent_size;              //内存块中实体大小
	int used_num;              //已使用个数
	int blk_size;
	struct mem_block_s *next;
} mem_block_t;

typedef struct mem_pool_s {
	mem_block_t *head;
	mem_block_t *tail;
	int ent_size;              //内存池中实体大小
	int ent_num;               //内存块中实体个数
	int total_size;
} mem_pool_t;


void *malloc_node(mem_pool_t *pool);
int free_node(mem_pool_t *pool, void *node);
mem_pool_t *create_memory_pool(int ent_size, int ent_num);

#endif //__MEM_POOL_H__

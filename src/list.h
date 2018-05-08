#ifndef __LIST_H__
#define __LIST_H__

typedef struct list_node_s {
    struct list_node_s *next;
    struct list_node_s *prev;
    void *ptr;
} list_node_t;

typedef struct list_s {
    list_node_t *head;
    list_node_t *tail;
    void (*free_func)(void *ptr);
    int (*compare_func)(void *ptr1, void *ptr2);
    int len;
} list_t;

#define LIST_HEAD(l)     ((l)->head) 
#define LIST_TAIL(l)     ((l)->tail)
#define LIST_LEN(l)  ((l)->len)

#define LIST_NEXT_NODE(n) ((n)->next)
#define LIST_PREV_NODE(n) ((n)->prev)
#define LIST_NODE_DATA(n) ((n)->ptr)

#define LIST_SET_FREE(l,f) (l)->free_func = (f))
#define LIST_SET_COMPARE(l,f) ((l)->compare_func = (f))

#define LIST_FOREACH(l,it) \
    for (list_node_t * it = LIST_HEAD(l); it != NULL; it = LIST_NEXT_NODE(it)) \

list_t *create_list(void);
void destroy_list(list_t * list);
void list_add_tail(list_t * list, void *ptr);
void list_add_head(list_t *list, void *ptr);
void list_insert_node(list_t *list, list_node_t *old_node, void *ptr, int after);
void list_delete_node(list_t *list, list_node_t *node);

#endif //__LIST_H__

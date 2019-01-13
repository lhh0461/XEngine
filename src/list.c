#include <stdlib.h>
#include <stdio.h>
#include "list.h"

//mem_pool_t *list_pool = NULL;
//mem_pool_t *node_pool = NULL;

list_t *create_list(void)
{
    list_t * list = NULL;
    list = malloc(sizeof(*list));
    if (list == NULL) return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    list->free_func = NULL;
    list->compare_func = NULL;

    return list;
}

void destroy_list(list_t * list)
{
    if (list == NULL) return;
    
    LIST_FOREACH(list, node) {
        if (list->free_func) {
            list->free_func(LIST_NODE_DATA(node));
        }
        free(node); 
    }
    free(list); 
}

static list_node_t * _new_node(void *ptr)
{
    list_node_t * node = NULL;

    node = malloc(sizeof(*node)); 
    if (node == NULL) return NULL;

    LIST_PREV_NODE(node) = NULL;
    LIST_NEXT_NODE(node) = NULL;
    LIST_NODE_DATA(node) = ptr;

    return node;
}

void list_add_tail(list_t * list, void *ptr)
{
    list_node_t *node = _new_node(ptr);
    if (node == NULL) return;
    if (LIST_LEN(list) == 0) {
        LIST_HEAD(list) = node;
        LIST_TAIL(list) = node;
    } else {
        node->next = NULL;
        node->prev = LIST_TAIL(list);
        LIST_TAIL(list)->next = node;
        LIST_TAIL(list) = node;
    }
    LIST_LEN(list)++;
}

void list_add_head(list_t *list, void *ptr)
{
    list_node_t *node = _new_node(ptr);
    if (node == NULL) return;
    if (LIST_LEN(list) == 0) {
        LIST_HEAD(list) = node;
        LIST_TAIL(list) = node;
    } else {
        node->next = LIST_HEAD(list);
        node->prev = NULL;
        LIST_HEAD(list)->prev = node;
        LIST_HEAD(list) = node;
    }
    LIST_LEN(list)++;
}

void list_insert_node(list_t *list, list_node_t *old_node, void *ptr, int after)
{
    list_node_t *node = _new_node(ptr);
    if (after) {
       node->next = old_node->next; 
       old_node->next = node; 
       node->prev = old_node;
       if (node->next == NULL) LIST_TAIL(list) = node;
       else node->next->prev = node;
    } else {
       node->prev = old_node->prev; 
       old_node->prev = node; 
       node->next = old_node;
       if (node->prev == NULL) LIST_HEAD(list) = node;
       else node->prev->next = node;
    }
}

void list_delete_node(list_t *list, list_node_t *node)
{
    if (node->prev) {
        node->prev->next  = node->next;
    } else {
        LIST_HEAD(list) = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        LIST_TAIL(list) = node->prev;
    }

    if (list->free_func) list->free_func(node->ptr);
    free(node);
    LIST_LEN(list)--;
}

list_node_t * list_find_node(list_t *list, void *ptr)
{
    LIST_FOREACH(list, node) {
        if (list->compare_func) {
            if (list->compare_func(LIST_NODE_DATA(node), ptr)) {
                return node;
            }
        } else if (LIST_NODE_DATA(node) == ptr) {
            return node;
        }
    }
    return NULL;
}
    
/*
struct tnode {
    int score;
    int class;
};
struct tlist {
    int aaa;
    list_t *l;
};

int compare_func(void *ptr1, void *ptr2)
{
    struct tnode *tnode1 = (struct tnode *) ptr1;
    struct tnode *tnode2 = (struct tnode *) ptr2;
    if (tnode1->score == tnode2->score) return 1;
    return 0;
}

int main()
{
    
    struct tlist *listp = malloc(sizeof(*listp));
    listp->l = create_list();
    listp->aaa = 500;
    LIST_SET_COMPARE(listp->l, compare_func);

    
    for (int i = 0; i < 10; i++) {
        struct tnode *n = malloc(sizeof(*n));
        n->score = i;
        n->class = i % 3;
        list_add_tail(listp->l, n);
    }
    LIST_FOREACH(listp->l, node) {
        struct tnode *t = LIST_NODE_DATA(node);
        printf("score=%d,class=%d\n", t->score, t->class);
    }

    list_delete_node(listp->l, LIST_TAIL(listp->l));
    printf("--------------------\n");
    struct tnode t = {8,2};
    list_node_t * fnode = list_find_node(listp->l, &t);
    //list_delete_node(listp->l, fnode);
    struct tnode t2 = {1111,2};
    list_insert_node(listp->l, fnode, &t2, 1);

    LIST_FOREACH(listp->l, node) {
        struct tnode *t = LIST_NODE_DATA(node);
        printf("score=%d,class=%d\n", t->score, t->class);
    }
    destroy_list(listp->l);
}
*/

#ifndef _LL__H
#define _LL__H

#include <stdlib.h>

typedef struct list_node
{
   struct list_node* next;
   void* val;
} list_node;

typedef struct linked_list
{
   unsigned int count;
   list_node* head;
} linked_list;

typedef int(*ll_cmpfunc)(void* list_item, const void* your_item);

linked_list* ll_init(void);
unsigned int ll_size(const linked_list* ll);
void ll_add(linked_list* ll, void* val); // adds an item to the tail of the list
void* ll_remove(linked_list* ll, const void* data, ll_cmpfunc cmp); // find an item with a user-defined comparator, and remove it from the list
void* ll_pop(linked_list* ll); // pops the item at the head of the list and returns it
void ll_push(linked_list* ll, void* val);
const list_node* ll_iter_start(const linked_list* ll);
void ll_insert(list_node* here, void* val);

#endif // _LL__H




/* file: list.h */

#include<stddef.h>
#include<stdint.h>

#ifndef LIST_H
#define LIST_H


#define LIST_ITEM_INIT(item, type){	\
	item.next = NULL;				\
	item.prev = NULL;				\
	item.holder = (void*)type;		\
};

#define LIST_INIT(list){		 	\
	list.size = 0;					\
	list.head = NULL;				\
	list.tail = NULL;				\
};

#define LIST_INITIAL_CONTENT {		\
	.size = 0,						\
	.head = NULL,					\
	.tail = NULL,					\
};

#define LIST_ITEM_HOLDER(type, item) (type)item->holder;

#define LIST_IS_EMPTY(list) (list.size == 0)

/*
 * List item that holds pointer to an actual data
 * */
struct list_item{
    
    void              *holder;
    struct list_item 	*next;
    struct list_item 	*prev;
};

/*
 * Doubly linked list for maintaining list items
 * */
struct list{

    struct list_item *head;
    struct list_item *tail;
    uint8_t           size;
};


/*
 * Insert an item to a back of the list
 * if succeeds returns pointer to a list's tail
 * else returns NULL
 * */
struct list_item *list_insert_back(struct list *list, struct list_item *item);


/*
 * Remove an item from the front of the list
 * if succeeds returns pointer to a removed item
 * else returns NULL
 * */
struct list_item *list_remove_front(struct list *list);


/*
 * Remove an item from the back of the list
 * If success returns pointer to removed item
 * else returns NULL
 * */
struct list_item *list_remove_back(struct list *list);


/*
 * Removes/pops item from arbitrary position in the list
 * */
struct list_item* list_remove(struct list *list, struct list_item *item);



#endif /*LIST_H*/


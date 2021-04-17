
/*
 * pavos_list.h
 * */

#ifndef PAVOS_LIST_H_
#define PAVOS_LIST_H_

#include<stddef.h>
#include<stdint.h>


#define m_item_init(item, type){		\
	item.next = NULL;			\
	item.prev = NULL;			\
	item.parent = (void*)type;		\
};

#define m_list_init(list){			\
	list.size = 0;				\
	list.head = NULL;			\
	list.tail = NULL;			\
};

#define m_list_initial_content {		\
	.size = 0,				\
	.head = NULL,				\
	.tail = NULL,				\
};

#define m_item_parent(type, item) 	(type)item->parent;
#define m_list_is_empty(list) 		(list.size == 0)

/*
 * List item that holds pointer to an actual data
 * */
struct _item{
    
	void	      *parent;
	struct _item	*next;
	struct _item	*prev;
};

/*
 * Doubly linked list for maintaining list items
 * */
struct _list{

	struct _item	*head;
	struct _item	*tail;
	uint8_t		 size;
};

/*
 * Insert an item to a back of the list
 * if succeeds returns pointer to a list's tail
 * else returns NULL
 * */
struct _item *_list_insert_back(struct _list *list, struct _item *item);


/*
 * Remove an item from the front of the list
 * if succeeds returns pointer to a removed item
 * else returns NULL
 * */
struct _item *_list_remove_front(struct _list *list);


/*
 * Remove an item from the back of the list
 * If success returns pointer to removed item
 * else returns NULL
 * */
struct _item *_list_remove_back(struct _list *list);


/*
 * Removes/pops item from arbitrary position in the list
 * */
struct _item *_list_remove(struct _list *list, struct _item *item);



#endif /*PAVOS_LIST_H*/


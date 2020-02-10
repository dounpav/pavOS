
/* 
 * file: list.c
 * */

#include"list.h"


struct list_item* list_insert_back(struct list *list, struct list_item *item){

	if(list == NULL) return NULL;
	if(item == NULL) return NULL;

	item->next = NULL;
	item->prev = NULL;

	if(list->size == 0){
		list->head = item;
		list->tail = item;
		list->size++;
	}
	else{

		struct list_item *old_tail = list->tail;
		list->tail = item;
		item->prev = old_tail;
		old_tail->next = item;
		list->size++;
	}

	return list->tail;
}

struct list_item* list_remove_front(struct list *list){

	if(list == NULL) return NULL;

	if(list->size == 0) return NULL;

	struct list_item *old_head = list->head;
	struct list_item *new_head = old_head->next;

	if(new_head != NULL){
		new_head->prev = NULL;

		if(list->tail == old_head){
			list->tail = new_head;
		}
		list->head = new_head;
	}
	else{
		list->head = NULL;
		list->tail = NULL;
	}
	old_head->next = NULL;
	old_head->prev = NULL;
	list->size--;

	return old_head;
}


struct list_item *list_remove_back(struct list *list){

	if(list == NULL) return NULL;
	if(list->size == 0) return NULL;

	struct list_item *old_tail = list->tail;
	struct list_item *new_tail = old_tail->prev;

	if(new_tail == NULL){
		list->head = NULL;
		list->tail = NULL;

	}
	else{
		new_tail->next = NULL;
		list->tail = new_tail;
	}

	old_tail->next = NULL;
	old_tail->prev = NULL;

	list->size--;

	return old_tail;
}

struct list_item* list_remove(struct list *list, struct list_item *item){

	if(item == NULL) return NULL;

	/*
	 * the item to be removed is the first item of the list
	 * */
	 if(list->head == item){
		 return list_remove_front(list);
	 }
	/*
	 * the item to be removed is the last item of the list
	 * */
	 else if(list->tail == item){
		 return list_remove_back(list);
	 }

	 /*
	  * the item to be removed is somewhere else in the list
	  * */
	 else{

		 if(list->head == NULL || list->tail == NULL){
			 return NULL;
		 }

		 struct list_item *prv_item = item->prev;
		 struct list_item *nxt_item = item->next;

		 prv_item->next = nxt_item;
		 nxt_item->prev = prv_item;

		 item->next = NULL;
		 item->prev = NULL;
		 list->size--;

		 return item;

	 }
}



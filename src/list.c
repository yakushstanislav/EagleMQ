/*
   Copyright (c) 2012, Stanislav Yakush(st.yakush@yandex.ru)
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the EagleMQ nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Taken from Redis */

#include <stdlib.h>

#include "eagle.h"
#include "list.h"
#include "xmalloc.h"

List *list_create(void)
{
	List *list;

	list = (List*)xmalloc(sizeof(*list));

	list->head = list->tail = NULL;
	list->len = 0;
	list->dup = NULL;
	list->free = NULL;
	list->match = NULL;

	return list;
}

void list_release(List *list)
{
	unsigned int len;
	ListNode *current, *next;

	current = list->head;
	len = list->len;

	while (len--) {
		next = current->next;

		if (list->free) {
			list->free(current->value);
		}

		xfree(current);
		current = next;
	}

	xfree(list);
}

List *list_add_value_head(List *list, void *value)
{
	ListNode *node;

	node = (ListNode*)xmalloc(sizeof(*node));

	node->value = value;

	if (list->len == 0) {
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = NULL;
		node->next = list->head;
		list->head->prev = node;
		list->head = node;
	}

	list->len++;

	return list;
}

List *list_add_value_tail(List *list, void *value)
{
	ListNode *node;

	node = (ListNode*)xmalloc(sizeof(*node));

	node->value = value;

	if (list->len == 0) {
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = list->tail;
		node->next = NULL;
		list->tail->next = node;
		list->tail = node;
	}

	list->len++;

	return list;
}

int list_delete_value(List *list, void *value)
{
	ListNode *node;

	node = list_search_node(list, value);

	if (!node) {
		return EG_STATUS_ERR;
	}

	list_delete_node(list, node);

	return EG_STATUS_OK;
}

List *list_insert_value(List *list, ListNode *old, void *value, int after)
{
	ListNode *node;

	node = (ListNode*)xmalloc(sizeof(*node));

	node->value = value;

	if (after) {
		node->prev = old;
		node->next = old->next;

		if (list->tail == old) {
			list->tail = node;
		}
	} else {
		node->next = old;
		node->prev = old->prev;

		if (list->head == old) {
			list->head = node;
		}
	}

	if (node->prev != NULL) {
		node->prev->next = node;
	}

	if (node->next != NULL) {
		node->next->prev = node;
	}

	list->len++;

	return list;
}

void list_delete_node(List *list, ListNode *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		list->head = node->next;
	}

	if (node->next) {
		node->next->prev = node->prev;
	} else {
		list->tail = node->prev;
	}

	if (list->free) {
		list->free(node->value);
	}

	xfree(node);
	list->len--;
}

ListIterator *list_get_iterator(List *list, int direction)
{
	ListIterator *iter;

	iter = (ListIterator*)xmalloc(sizeof(*iter));

	if (direction == EG_START_HEAD) {
		iter->next = list->head;
	} else {
		iter->next = list->tail;
	}

	iter->direction = direction;

	return iter;
}

void list_release_iterator(ListIterator *iter)
{
	xfree(iter);
}

ListNode *list_next_node(ListIterator *iter)
{
	ListNode *current = iter->next;

	if (current != NULL) {
		if (iter->direction == EG_START_HEAD) {
			iter->next = current->next;
		} else {
			iter->next = current->prev;
		}
	}

	return current;
}

ListNode *list_search_node(List *list, void *value)
{
	ListIterator *iter;
	ListNode *node;

	iter = list_get_iterator(list, EG_START_HEAD);

	while ((node = list_next_node(iter)) != NULL) {
		if (list->match) {
			if (list->match(node->value, value)) {
				list_release_iterator(iter);
				return node;
			}
		} else {
			if (node->value == value) {
				list_release_iterator(iter);
				return node;
			}
		}
	}

	list_release_iterator(iter);

	return NULL;
}

void list_rotate(List *list)
{
	ListNode *tail = list->tail;

	if (EG_LIST_LENGTH(list) <= 1)
		return;

	list->tail = tail->prev;
	list->tail->next = NULL;

	list->head->prev = tail;
	tail->prev = NULL;
	tail->next = list->head;
	list->head = tail;
}

void list_rewind(List *list, ListIterator *iter)
{
	iter->next = list->head;
	iter->direction = EG_START_HEAD;
}

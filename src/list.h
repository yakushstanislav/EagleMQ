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

#ifndef __LIST_LIB_H__
#define __LIST_LIB_H__

#define EG_START_HEAD 0
#define EG_START_TAIL 1

#define EG_LIST_LENGTH(l) ((l)->len)
#define EG_LIST_FIRST(l) ((l)->head)
#define EG_LIST_LAST(l) ((l)->tail)
#define EG_LIST_PREV_NODE(n) ((n)->prev)
#define EG_LIST_NEXT_NODE(n) ((n)->next)
#define EG_LIST_NODE_VALUE(n) ((n)->value)

#define EG_LIST_SET_DUP_METHOD(l, m) ((l)->dup = (m))
#define EG_LIST_SET_FREE_METHOD(l, m) ((l)->free = (m))
#define EG_LIST_SET_MATCH_METHOD(l, m) ((l)->match = (m))

#define EG_LIST_GET_DUP_METHOD(l) ((l)->dup)
#define EG_LIST_GET_FREE_METHOD(l) ((l)->free)
#define EG_LIST_GET_MATCH_METHOD(l) ((l)->match)

typedef struct ListNode {
	struct ListNode *prev;
	struct ListNode *next;
	void *value;
} ListNode;

typedef struct ListIterator {
	ListNode *next;
	int direction;
} ListIterator;

typedef struct List {
	ListNode *head;
	ListNode *tail;
	void *(*dup)(void *ptr);
	void (*free)(void *ptr);
	int (*match)(void *ptr, void *value);
	unsigned int len;
} List;

List *list_create(void);
void list_release(List *list);
List *list_add_value_head(List *list, void *value);
List *list_add_value_tail(List *list, void *value);
int list_delete_value(List *list, void *value);
List *list_insert_value(List *list, ListNode *old, void *value, int after);
void list_delete_node(List *list, ListNode *node);
ListIterator *list_get_iterator(List *list, int direction);
void list_release_iterator(ListIterator *iter);
ListNode *list_next_node(ListIterator *iter);
ListNode *list_search_node(List *list, void *value);
void list_rewind(List *list, ListIterator *iter);

#endif

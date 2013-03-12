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

#include <stdlib.h>

#include "eagle.h"
#include "keylist.h"
#include "xmalloc.h"

Keylist *keylist_create(void)
{
	Keylist *keylist;

	keylist = (Keylist*)xmalloc(sizeof(*keylist));

	keylist->head = keylist->tail = NULL;
	keylist->len = 0;
	keylist->free = NULL;
	keylist->match = NULL;

	return keylist;
}

void keylist_release(Keylist *keylist)
{
	unsigned int len;
	KeylistNode *current, *next;

	current = keylist->head;
	len = keylist->len;

	while (len--) {
		next = current->next;

		if (keylist->free) {
			keylist->free(current->key, current->value);
		}

		xfree(current);
		current = next;
	}

	xfree(keylist);
}

KeylistNode *keylist_get_value(Keylist *keylist, void *key)
{
	KeylistIterator iter;
	KeylistNode *node;

	keylist_rewind(keylist, &iter);
	while ((node = keylist_next_node(&iter)) != NULL)
	{
		if (keylist->match) {
			if (keylist->match(node->key, key)) {
				return node;
			}
		} else {
			if (node->key == key) {
				return node;
			}
		}
	}

	return NULL;
}

Keylist *keylist_set_value(Keylist *keylist, void *key, void *value)
{
	KeylistNode *node = keylist_get_value(keylist, key);

	if (node != NULL) {
		node->value = value;
		return keylist;
	}

	node = (KeylistNode*)xmalloc(sizeof(*node));

	node->key = key;
	node->value = value;

	if (keylist->len == 0) {
		keylist->head = keylist->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = keylist->tail;
		node->next = NULL;
		keylist->tail->next = node;
		keylist->tail = node;
	}

	keylist->len++;

	return keylist;
}

void keylist_delete_node(Keylist *keylist, KeylistNode *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		keylist->head = node->next;
	}

	if (node->next) {
		node->next->prev = node->prev;
	} else {
		keylist->tail = node->prev;
	}

	if (keylist->free) {
		keylist->free(node->key, node->value);
	}

	xfree(node);
	keylist->len--;
}

KeylistNode *keylist_next_node(KeylistIterator *iter)
{
	KeylistNode *current = iter->next;

	if (current != NULL) {
		iter->next = current->next;
	}

	return current;
}

void keylist_rewind(Keylist *keylist, KeylistIterator *iter)
{
	iter->next = keylist->head;
}
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

#ifndef __KEYLIST_LIB_H__
#define __KEYLIST_LIB_H__

#define EG_KEYLIST_LENGTH(l) ((l)->len)
#define EG_KEYLIST_FIRST(l) ((l)->head)
#define EG_KEYLIST_LAST(l) ((l)->tail)
#define EG_KEYLIST_PREV_NODE(n) ((n)->prev)
#define EG_KEYLIST_NEXT_NODE(n) ((n)->next)
#define EG_KEYLIST_NODE_KEY(n) ((n)->key)
#define EG_KEYLIST_NODE_VALUE(n) ((n)->value)

#define EG_KEYLIST_SET_FREE_METHOD(l, m) ((l)->free = (m))
#define EG_KEYLIST_SET_MATCH_METHOD(l, m) ((l)->match = (m))

#define EG_KEYLIST_GET_FREE_METHOD(l) ((l)->free)
#define EG_KEYLIST_GET_MATCH_METHOD(l) ((l)->match)

typedef struct KeylistNode {
	struct KeylistNode *prev;
	struct KeylistNode *next;
	void *key;
	void *value;
} KeylistNode;

typedef struct KeylistIterator {
	KeylistNode *next;
} KeylistIterator;

typedef struct Keylist {
	KeylistNode *head;
	KeylistNode *tail;
	void (*free)(void *key, void *value);
	int (*match)(void *key1, void *key2);
	unsigned int len;
} Keylist;

Keylist *keylist_create(void);
void keylist_release(Keylist *keylist);
KeylistNode *keylist_get_value(Keylist *keylist, void *key);
Keylist *keylist_set_value(Keylist *keylist, void *key, void *value);
void keylist_delete_node(Keylist *keylist, KeylistNode *node);
KeylistNode *keylist_next_node(KeylistIterator *iter);
void keylist_rewind(Keylist *keylist, KeylistIterator *iter);

#endif

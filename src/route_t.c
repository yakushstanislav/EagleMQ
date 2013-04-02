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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "eagle.h"
#include "route_t.h"
#include "queue_t.h"
#include "protocol.h"
#include "keylist.h"
#include "xmalloc.h"
#include "utils.h"

static void unbind_queues_key_route_t(Route_t *route, List *queues, const char *key);
static void unbind_queues_route_t(Route_t* route);

static void free_route_keylist_handler(void *key, void *value);
static int match_route_keylist_handler(void *key1, void *key2);

Route_t *create_route_t(const char *name, uint32_t flags)
{
	Route_t *route = (Route_t*)xmalloc(sizeof(*route));

	memcpy(route->name, name, strlenz(name));
	route->flags = flags;

	route->auto_delete = 0;
	route->round_robin = 0;

	if (BIT_CHECK(route->flags, EG_ROUTE_AUTODELETE_FLAG)) {
		route->auto_delete = 1;
	}

	if (BIT_CHECK(route->flags, EG_ROUTE_ROUND_ROBIN_FLAG)) {
		route->round_robin = 1;
	}

	route->keys = keylist_create();

	EG_KEYLIST_SET_FREE_METHOD(route->keys, free_route_keylist_handler);
	EG_KEYLIST_SET_MATCH_METHOD(route->keys, match_route_keylist_handler);

	return route;
}

void delete_route_t(Route_t *route)
{
	unbind_queues_route_t(route);

	keylist_release(route->keys);

	xfree(route);
}

int push_message_route_t(Route_t *route, const char *key, Object *msg, uint32_t expiration)
{
	KeylistNode *keylist_node;
	ListNode *list_node;
	ListIterator list_iterator;
	List *list;
	Queue_t *queue_t;
	int status = EG_STATUS_OK;

	keylist_node = keylist_get_value(route->keys, (void*)key);
	if (!keylist_node) {
		return EG_STATUS_ERR;
	}

	list = EG_KEYLIST_NODE_VALUE(keylist_node);

	if (route->round_robin)
	{
		list_rotate(list);

		queue_t = EG_LIST_NODE_VALUE(EG_LIST_FIRST(list));

		if (push_message_queue_t(queue_t, msg, expiration) != EG_STATUS_OK)
			status = EG_STATUS_ERR;

		increment_references_count(msg);
	}
	else
	{
		list_rewind(list, &list_iterator);
		while ((list_node = list_next_node(&list_iterator)) != NULL)
		{
			queue_t = EG_LIST_NODE_VALUE(list_node);

			if (push_message_queue_t(queue_t, msg, expiration) != EG_STATUS_OK)
				status = EG_STATUS_ERR;

			increment_references_count(msg);
		}
	}

	return status;
}

void bind_route_t(Route_t *route, Queue_t *queue_t, const char *key)
{
	List *list;
	KeylistNode *node = keylist_get_value(route->keys, (void*)key);
	int link = 0;

	if (!node)
	{
		list = list_create();
		keylist_set_value(route->keys, xstrdup(key), list);
		link = 1;
	}
	else
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		if (!list_search_node(list, queue_t))
			link = 1;
	}

	if (link)
	{
		list_add_value_tail(list, queue_t);
		link_queue_route_t(queue_t, route, key);
	}
}

int unbind_route_t(Route_t *route, Queue_t *queue_t, const char *key)
{
	List *list;
	KeylistNode *node = keylist_get_value(route->keys, (void*)key);

	if (!node) {
		return EG_STATUS_ERR;
	}

	list = EG_KEYLIST_NODE_VALUE(node);
	list_delete_value(list, queue_t);

	if (!EG_LIST_LENGTH(list)) {
		keylist_delete_node(route->keys, node);
	}

	unlink_queue_route_t(queue_t, route, key);

	if (route->auto_delete)
	{
		if (EG_KEYLIST_LENGTH(route->keys) == 0) {
			list_delete_value(server->routes, route);
		}
	}

	return EG_STATUS_OK;
}

Route_t *find_route_t(List *list, const char *name)
{
	Route_t *route;
	ListNode *node;
	ListIterator iterator;

	list_rewind(list, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		route = EG_LIST_NODE_VALUE(node);
		if (!strcmp(route->name, name)) {
			return route;
		}
	}

	return NULL;
}

uint32_t get_queue_number_route_t(Route_t *route)
{
	KeylistIterator iterator;
	KeylistNode *node;
	List *list;
	uint32_t queues = 0;

	keylist_rewind(route->keys, &iterator);
	while ((node = keylist_next_node(&iterator)) != NULL)
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		queues += EG_LIST_LENGTH(list);
	}

	return queues;
}

static inline void unbind_queues_key_route_t(Route_t *route, List *queues, const char *key)
{
	ListIterator iterator;
	ListNode *node;
	Queue_t *queue_t;

	list_rewind(queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);

		unlink_queue_route_t(queue_t, route, key);
	}
}

static void unbind_queues_route_t(Route_t *route)
{
	KeylistIterator iterator;
	KeylistNode *node;

	keylist_rewind(route->keys, &iterator);
	while ((node = keylist_next_node(&iterator)) != NULL)
	{
		unbind_queues_key_route_t(route, EG_KEYLIST_NODE_VALUE(node), EG_KEYLIST_NODE_KEY(node));
	}
}

void free_route_list_handler(void *ptr)
{
	delete_route_t(ptr);
}

static void free_route_keylist_handler(void *key, void *value)
{
	xfree(key);
	list_release(value);
}

static int match_route_keylist_handler(void *key1, void *key2)
{
	return !strcmp(key1, key2);
}

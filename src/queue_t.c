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
#include "queue_t.h"
#include "route_t.h"
#include "protocol.h"
#include "object.h"
#include "message.h"
#include "handlers.h"
#include "queue.h"
#include "list.h"
#include "keylist.h"
#include "xmalloc.h"
#include "utils.h"

static void eject_clients_queue_t(Queue_t *queue_t);
static void eject_routes_key_queue_t(Queue_t *queue_t, List *routes, const char *key);
static void eject_routes_queue_t(Queue_t *queue_t);

static void free_route_keylist_handler(void *key, void *value);
static int match_route_keylist_handler(void *key1, void *key2);

Queue_t *create_queue_t(const char *name, uint32_t max_msg, uint32_t max_msg_size, uint32_t flags)
{
	Queue_t *queue_t = (Queue_t*)xmalloc(sizeof(*queue_t));

	memcpy(queue_t->name, name, strlenz(name));

	queue_t->max_msg = max_msg;
	queue_t->max_msg_size = max_msg_size;
	queue_t->flags = flags;

	queue_t->auto_delete = 0;
	queue_t->force_push = 0;
	queue_t->round_robin = 0;

	if (BIT_CHECK(queue_t->flags, EG_QUEUE_AUTODELETE_FLAG)) {
		queue_t->auto_delete = 1;
	}

	if (BIT_CHECK(queue_t->flags, EG_QUEUE_FORCE_PUSH_FLAG)) {
		queue_t->force_push = 1;
	}

	if (BIT_CHECK(queue_t->flags, EG_QUEUE_ROUND_ROBIN_FLAG)) {
		queue_t->round_robin = 1;
	}

	queue_t->queue = queue_create();
	queue_t->expire_messages = list_create();
	queue_t->confirm_messages = list_create();
	queue_t->declared_clients = list_create();
	queue_t->subscribed_clients_msg = list_create();
	queue_t->subscribed_clients_notify = list_create();
	queue_t->routes = keylist_create();

	EG_QUEUE_SET_FREE_METHOD(queue_t->queue, free_message_list_handler);
	EG_KEYLIST_SET_FREE_METHOD(queue_t->routes, free_route_keylist_handler);
	EG_KEYLIST_SET_MATCH_METHOD(queue_t->routes, match_route_keylist_handler);

	return queue_t;
}

void delete_queue_t(Queue_t *queue_t)
{
	eject_clients_queue_t(queue_t);
	eject_routes_queue_t(queue_t);

	list_release(queue_t->expire_messages);
	list_release(queue_t->confirm_messages);
	list_release(queue_t->declared_clients);
	list_release(queue_t->subscribed_clients_msg);
	list_release(queue_t->subscribed_clients_notify);

	keylist_release(queue_t->routes);

	queue_release(queue_t->queue);

	xfree(queue_t);
}

static int process_subscribed_clients(Queue_t *queue_t, Message *msg)
{
	ListNode *node;
	ListIterator iterator;
	EagleClient *client;
	int processed = 0;

	if (EG_QUEUE_LENGTH(queue_t->subscribed_clients_msg))
	{
		if (queue_t->round_robin)
		{
			list_rotate(queue_t->subscribed_clients_msg);

			client = EG_LIST_NODE_VALUE(EG_LIST_FIRST(queue_t->subscribed_clients_msg));
			queue_client_event_message(client, queue_t, msg);

			processed++;
		}
		else
		{
			list_rewind(queue_t->subscribed_clients_msg, &iterator);
			while ((node = list_next_node(&iterator)) != NULL)
			{
				client = EG_LIST_NODE_VALUE(node);
				queue_client_event_message(client, queue_t, msg);

				processed++;
			}
		}
	}

	if (EG_QUEUE_LENGTH(queue_t->subscribed_clients_notify))
	{
		list_rewind(queue_t->subscribed_clients_notify, &iterator);
		while ((node = list_next_node(&iterator)) != NULL)
		{
			client = EG_LIST_NODE_VALUE(node);
			queue_client_event_notify(client, queue_t);
		}
	}

	return processed;
}

int push_message_queue_t(Queue_t *queue_t, Object *data, uint32_t expiration)
{
	Message *msg;
	uint64_t tag = make_message_tag(server->msg_counter++, server->now_timems);

	msg = create_message(data, tag, expiration);

	if (EG_MESSAGE_SIZE(msg) > queue_t->max_msg_size) {
		return EG_STATUS_ERR;
	}

	if (process_subscribed_clients(queue_t, msg)) {
		release_message(msg);
		return EG_STATUS_OK;
	}

	if (EG_QUEUE_LENGTH(queue_t->queue) >= queue_t->max_msg)
	{
		if (queue_t->force_push) {
			pop_message_queue_t(queue_t, 0);
		} else {
			return EG_STATUS_ERR;
		}
	}

	queue_push_value_head(queue_t->queue, msg);

	if (msg->expiration)
	{
		list_add_value_tail(queue_t->expire_messages, msg);
		EG_MESSAGE_SET_DATA(msg, 0, EG_LIST_LAST(queue_t->expire_messages));
		EG_MESSAGE_SET_DATA(msg, 1, EG_QUEUE_FIRST(queue_t->queue));
	}

	return EG_STATUS_OK;
}

Message *get_message_queue_t(Queue_t *queue_t)
{
	return queue_get_value(queue_t->queue);
}

void pop_message_queue_t(Queue_t *queue_t, uint32_t timeout)
{
	Message *msg = queue_pop_value(queue_t->queue);

	if (msg->expiration) {
		list_delete_node(queue_t->expire_messages, EG_MESSAGE_GET_DATA(msg, 0));
	}

	if (timeout) {
		EG_MESSAGE_SET_CONFIRM_TIME(msg, server->now_timems + timeout);
		list_add_value_tail(queue_t->confirm_messages, msg);
	} else {
		release_message(msg);
	}
}

int confirm_message_queue_t(Queue_t *queue_t, uint64_t tag)
{
	ListNode *node;
	ListIterator iterator;
	Message *msg;

	list_rewind(queue_t->confirm_messages, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		msg = EG_LIST_NODE_VALUE(node);

		if (EG_MESSAGE_GET_TAG(msg) == tag)
		{
			list_delete_node(queue_t->confirm_messages, node);
			release_message(msg);
			return EG_STATUS_OK;
		}
	}

	return EG_STATUS_ERR;
}

Queue_t *find_queue_t(List *list, const char *name)
{
	Queue_t *queue_t;
	ListNode *node;
	ListIterator iterator;

	list_rewind(list, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);
		if (!strcmp(queue_t->name, name)) {
			return queue_t;
		}
	}

	return NULL;
}

void rename_queue_t(Queue_t *queue_t, const char *name)
{
	memcpy(queue_t->name, name, strlenz(name));
}

uint32_t get_declared_clients_queue_t(Queue_t *queue_t)
{
	return EG_LIST_LENGTH(queue_t->declared_clients);
}

uint32_t get_subscribed_clients_queue_t(Queue_t *queue_t)
{
	return EG_LIST_LENGTH(queue_t->subscribed_clients_msg) +
		EG_LIST_LENGTH(queue_t->subscribed_clients_notify);
}

uint32_t get_size_queue_t(Queue_t *queue_t)
{
	return EG_QUEUE_LENGTH(queue_t->queue);
}

void purge_queue_t(Queue_t *queue_t)
{
	queue_purge(queue_t->queue);
}

void declare_client_queue_t(Queue_t *queue_t, EagleClient *client)
{
	list_add_value_tail(client->declared_queues, queue_t);
	list_add_value_tail(queue_t->declared_clients, client);
}

void undeclare_client_queue_t(Queue_t *queue_t, EagleClient *client)
{
	list_delete_value(client->declared_queues, queue_t);
	list_delete_value(queue_t->declared_clients, client);
}

void subscribe_client_queue_t(Queue_t *queue_t, EagleClient *client, uint32_t flags)
{
	list_add_value_tail(client->subscribed_queues, queue_t);

	if (!BIT_CHECK(flags, EG_QUEUE_CLIENT_NOTIFY_FLAG)) {
		list_add_value_tail(queue_t->subscribed_clients_msg, client);
	} else {
		list_add_value_tail(queue_t->subscribed_clients_notify, client);
	}
}

void unsubscribe_client_queue_t(Queue_t *queue_t, EagleClient *client)
{
	ListNode *node;
	ListIterator iterator;

	list_delete_value(client->subscribed_queues, queue_t);

	list_rewind(queue_t->subscribed_clients_msg, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		if (EG_LIST_NODE_VALUE(node) == client) {
			list_delete_node(queue_t->subscribed_clients_msg, node);
			return;
		}
	}

	list_rewind(queue_t->subscribed_clients_notify, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		if (EG_LIST_NODE_VALUE(node) == client) {
			list_delete_node(queue_t->subscribed_clients_notify, node);
		}
	}
}

void link_queue_route_t(Queue_t *queue_t, Route_t *route, const char *key)
{
	List *list;
	KeylistNode *node = keylist_get_value(queue_t->routes, (void*)key);

	if (!node)
	{
		list = list_create();
		keylist_set_value(queue_t->routes, xstrdup(key), list);
		list_add_value_tail(list, route);
	}
	else
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		if (!list_search_node(list, route)) {
			list_add_value_tail(list, route);
		}
	}
}

void unlink_queue_route_t(Queue_t *queue_t, Route_t *route, const char *key)
{
	List *list;
	KeylistNode *node = keylist_get_value(queue_t->routes, (void*)key);

	if (node)
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		list_delete_value(list, route);

		if (!EG_LIST_LENGTH(list)) {
			keylist_delete_node(queue_t->routes, node);
		}
	}
}

void process_queue_t(Queue_t *queue_t)
{
	if (queue_t->auto_delete)
	{
		if (EG_LIST_LENGTH(queue_t->declared_clients) == 0) {
			list_delete_value(server->queues, queue_t);
		}
	}
}

void process_expired_messages_queue_t(Queue_t *queue_t, uint32_t time)
{
	ListNode *node;
	ListIterator iterator;
	Message *msg;

	list_rewind(queue_t->expire_messages, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		msg = EG_LIST_NODE_VALUE(node);

		if (EG_MESSAGE_GET_EXPIRATION_TIME(msg) <= time) {
			list_delete_node(queue_t->expire_messages, node);
			queue_delete_node(queue_t->queue, EG_MESSAGE_GET_DATA(msg, 1));
		}
	}
}

void process_unconfirmed_messages_queue_t(Queue_t *queue_t, uint32_t time)
{
	ListNode *node;
	ListIterator iterator;
	Message *msg;

	list_rewind(queue_t->confirm_messages, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		msg = EG_LIST_NODE_VALUE(node);

		if (EG_MESSAGE_GET_CONFIRM_TIME(msg) <= time)
		{
			list_delete_node(queue_t->confirm_messages, node);

			if (msg->expiration)
			{
				if (EG_MESSAGE_GET_EXPIRATION_TIME(msg) <= time) {
					continue;
				}

				queue_push_value_tail(queue_t->queue, msg);

				list_add_value_tail(queue_t->expire_messages, msg);
				EG_MESSAGE_SET_DATA(msg, 0, EG_LIST_LAST(queue_t->expire_messages));
				EG_MESSAGE_SET_DATA(msg, 1, EG_QUEUE_LAST(queue_t->queue));
			}
			else
			{
				queue_push_value_tail(queue_t->queue, msg);
			}
		}
	}
}

static void eject_clients_queue_t(Queue_t *queue_t)
{
	ListNode *node;
	ListIterator iterator;
	EagleClient *client;

	list_rewind(queue_t->declared_clients, &iterator);
	while ((node = list_next_node(&iterator)) != NULL) {
		undeclare_client_queue_t(queue_t, EG_LIST_NODE_VALUE(node));
	}

	list_rewind(queue_t->subscribed_clients_msg, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		client = EG_LIST_NODE_VALUE(node);

		list_delete_value(client->subscribed_queues, queue_t);
		list_delete_node(queue_t->subscribed_clients_msg, node);
	}

	list_rewind(queue_t->subscribed_clients_notify, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		client = EG_LIST_NODE_VALUE(node);

		list_delete_value(client->subscribed_queues, queue_t);
		list_delete_node(queue_t->subscribed_clients_notify, node);
	}
}

static inline void eject_routes_key_queue_t(Queue_t *queue_t, List *routes, const char *key)
{
	ListIterator iterator;
	ListNode *node;
	Route_t *route;

	list_rewind(routes, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		route = EG_LIST_NODE_VALUE(node);

		unbind_route_t(route, queue_t, key);
	}
}

static void eject_routes_queue_t(Queue_t *queue_t)
{
	KeylistIterator iterator;
	KeylistNode *node;

	keylist_rewind(queue_t->routes, &iterator);
	while ((node = keylist_next_node(&iterator)) != NULL)
	{
		eject_routes_key_queue_t(queue_t, EG_KEYLIST_NODE_VALUE(node), EG_KEYLIST_NODE_KEY(node));
	}
}

void free_queue_list_handler(void *ptr)
{
	delete_queue_t(ptr);
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
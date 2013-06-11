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
#include <string.h>

#include "eagle.h"
#include "channel_t.h"
#include "handlers.h"
#include "protocol.h"
#include "keylist.h"
#include "xmalloc.h"
#include "utils.h"

static void eject_clients_channel_t(Channel_t *channel);

static void free_channel_keylist_handler(void *key, void *value);
static int match_channel_keylist_handler(void *key1, void *key2);

Channel_t *create_channel_t(const char *name, uint32_t flags)
{
	Channel_t *channel = (Channel_t*)xmalloc(sizeof(*channel));

	memcpy(channel->name, name, strlenz(name));
	channel->flags = flags;

	channel->auto_delete = 0;
	channel->round_robin = 0;

	if (BIT_CHECK(channel->flags, EG_CHANNEL_AUTODELETE_FLAG)) {
		channel->auto_delete = 1;
	}

	if (BIT_CHECK(channel->flags, EG_CHANNEL_ROUND_ROBIN_FLAG)) {
		channel->round_robin = 1;
	}

	channel->topics = keylist_create();
	channel->patterns = keylist_create();

	EG_KEYLIST_SET_FREE_METHOD(channel->topics, free_channel_keylist_handler);
	EG_KEYLIST_SET_MATCH_METHOD(channel->topics, match_channel_keylist_handler);

	EG_KEYLIST_SET_FREE_METHOD(channel->patterns, free_channel_keylist_handler);
	EG_KEYLIST_SET_MATCH_METHOD(channel->patterns, match_channel_keylist_handler);

	return channel;
}

void delete_channel_t(Channel_t *channel)
{
	eject_clients_channel_t(channel);

	keylist_release(channel->topics);
	keylist_release(channel->patterns);

	xfree(channel);
}

void publish_message_channel_t(Channel_t *channel, const char *topic, Object *msg)
{
	EagleClient *client;
	KeylistIterator keylist_iterator;
	KeylistNode *keylist_node;
	ListNode *list_node;
	ListIterator list_iterator;
	List *list;
	char *pattern;

	keylist_node = keylist_get_value(channel->topics, (void*)topic);
	if (keylist_node)
	{
		list = EG_KEYLIST_NODE_VALUE(keylist_node);

		if (channel->round_robin)
		{
			list_rotate(list);

			client = EG_LIST_NODE_VALUE(EG_LIST_FIRST(list));
			channel_client_event_message(client, channel, topic, msg);
		}
		else
		{
			list_rewind(list, &list_iterator);
			while ((list_node = list_next_node(&list_iterator)) != NULL)
			{
				client = EG_LIST_NODE_VALUE(list_node);
				channel_client_event_message(client, channel, topic, msg);
			}
		}
	}

	if (EG_KEYLIST_LENGTH(channel->patterns))
	{
		keylist_rewind(channel->patterns, &keylist_iterator);
		while ((keylist_node = keylist_next_node(&keylist_iterator)) != NULL)
		{
			pattern = EG_KEYLIST_NODE_KEY(keylist_node);

			if (pattern_match(topic, pattern, 0))
			{
				list = EG_KEYLIST_NODE_VALUE(keylist_node);

				list_rewind(list, &list_iterator);
				while ((list_node = list_next_node(&list_iterator)) != NULL)
				{
					client = EG_LIST_NODE_VALUE(list_node);
					channel_client_event_pattern_message(client, channel, topic, pattern, msg);
				}
			}
		}
	}
}

Channel_t *find_channel_t(List *list, const char *name)
{
	Channel_t *channel;
	ListNode *node;
	ListIterator iterator;

	list_rewind(list, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		channel = EG_LIST_NODE_VALUE(node);
		if (!strcmp(channel->name, name)) {
			return channel;
		}
	}

	return NULL;
}

void rename_channel_t(Channel_t *channel, const char *name)
{
	memcpy(channel->name, name, strlenz(name));
}

static void add_keylist_channel_t(Keylist *keylist, Channel_t *channel, KeylistNode *keylist_node)
{
	List *list;
	KeylistNode *node = keylist_get_value(keylist, channel);
	int add = 0;

	if (!node)
	{
		list = list_create();
		keylist_set_value(keylist, channel, list);
		add = 1;
	}
	else
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		if (!list_search_node(list, keylist_node))
			add = 1;
	}

	if (add)
	{
		list_add_value_tail(list, keylist_node);
	}
}

static int remove_keylist_channel_t(Keylist *keylist, Channel_t *channel, KeylistNode *keylist_node)
{
	List *list;
	KeylistNode *node = keylist_get_value(keylist, channel);

	if (!node) {
		return EG_STATUS_ERR;
	}

	list = EG_KEYLIST_NODE_VALUE(node);
	list_delete_value(list, keylist_node);

	if (!EG_LIST_LENGTH(list)) {
		list_release(list);
		keylist_delete_node(keylist, node);
	}

	return EG_STATUS_OK;
}

void subscribe_channel_t(Channel_t *channel, EagleClient *client, const char *topic)
{
	List *list;
	KeylistNode *node = keylist_get_value(channel->topics, (void*)topic);
	int subscribe = 0;

	if (!node)
	{
		list = list_create();
		keylist_set_value(channel->topics, xstrdup(topic), list);
		node = EG_KEYLIST_LAST(channel->topics);
		subscribe = 1;
	}
	else
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		if (!list_search_node(list, client))
			subscribe = 1;
	}

	if (subscribe)
	{
		list_add_value_tail(list, client);
		add_keylist_channel_t(client->subscribed_topics, channel, node);
	}
}

void psubscribe_channel_t(Channel_t *channel, EagleClient *client, const char *pattern)
{
	List *list;
	KeylistNode *node = keylist_get_value(channel->patterns, (void*)pattern);
	int subscribe = 0;

	if (!node)
	{
		list = list_create();
		keylist_set_value(channel->patterns, xstrdup(pattern), list);
		node = EG_KEYLIST_LAST(channel->patterns);
		subscribe = 1;
	}
	else
	{
		list = EG_KEYLIST_NODE_VALUE(node);
		if (!list_search_node(list, client))
			subscribe = 1;
	}

	if (subscribe)
	{
		list_add_value_tail(list, client);
		add_keylist_channel_t(client->subscribed_patterns, channel, node);
	}
}

int unsubscribe_channel_t(Channel_t *channel, EagleClient *client, const char *topic)
{
	List *list;
	KeylistNode *node = keylist_get_value(channel->topics, (void*)topic);

	if (!node) {
		return EG_STATUS_ERR;
	}

	list = EG_KEYLIST_NODE_VALUE(node);

	list_delete_value(list, client);
	remove_keylist_channel_t(client->subscribed_topics, channel, node);

	if (!EG_LIST_LENGTH(list)) {
		keylist_delete_node(channel->topics, node);
	}

	if (channel->auto_delete)
	{
		if (EG_KEYLIST_LENGTH(channel->topics) == 0 &&
			EG_KEYLIST_LENGTH(channel->patterns) == 0) {
			list_delete_value(server->channels, channel);
		}
	}

	return EG_STATUS_OK;
}

int punsubscribe_channel_t(Channel_t *channel, EagleClient *client, const char *pattern)
{
	List *list;
	KeylistNode *node = keylist_get_value(channel->patterns, (void*)pattern);

	if (!node) {
		return EG_STATUS_ERR;
	}

	list = EG_KEYLIST_NODE_VALUE(node);

	list_delete_value(list, client);
	remove_keylist_channel_t(client->subscribed_patterns, channel, node);

	if (!EG_LIST_LENGTH(list)) {
		keylist_delete_node(channel->patterns, node);
	}

	if (channel->auto_delete)
	{
		if (EG_KEYLIST_LENGTH(channel->topics) == 0 &&
			EG_KEYLIST_LENGTH(channel->patterns) == 0) {
			list_delete_value(server->channels, channel);
		}
	}

	return EG_STATUS_OK;
}

static void eject_clients_channel_t(Channel_t *channel)
{
	ListIterator list_iterator;
	ListNode *list_node;
	KeylistIterator keylist_iterator;
	KeylistNode *keylist_node;
	EagleClient *client;
	List *list;

	keylist_rewind(channel->topics, &keylist_iterator);
	while ((keylist_node = keylist_next_node(&keylist_iterator)) != NULL)
	{
		list = EG_KEYLIST_NODE_VALUE(keylist_node);

		list_rewind(list, &list_iterator);
		while ((list_node = list_next_node(&list_iterator)) != NULL)
		{
			client = EG_LIST_NODE_VALUE(list_node);
			unsubscribe_channel_t(channel, client, EG_KEYLIST_NODE_KEY(keylist_node));
		}
	}

	keylist_rewind(channel->patterns, &keylist_iterator);
	while ((keylist_node = keylist_next_node(&keylist_iterator)) != NULL)
	{
		list = EG_KEYLIST_NODE_VALUE(keylist_node);

		list_rewind(list, &list_iterator);
		while ((list_node = list_next_node(&list_iterator)) != NULL)
		{
			client = EG_LIST_NODE_VALUE(list_node);
			punsubscribe_channel_t(channel, client, EG_KEYLIST_NODE_KEY(keylist_node));
		}
	}
}

void free_channel_list_handler(void *ptr)
{
	delete_channel_t(ptr);
}

static void free_channel_keylist_handler(void *key, void *value)
{
	xfree(key);
	list_release(value);
}

static int match_channel_keylist_handler(void *key1, void *key2)
{
	return !strcmp(key1, key2);
}

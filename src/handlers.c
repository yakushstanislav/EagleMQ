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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <time.h>

#include "eagle.h"
#include "handlers.h"
#include "version.h"
#include "protocol.h"
#include "object.h"
#include "event.h"
#include "network.h"
#include "list.h"
#include "user.h"
#include "queue_t.h"
#include "storage.h"
#include "xmalloc.h"
#include "utils.h"

static int set_write_event(EagleClient *client);
static void add_response(EagleClient *client, void *data, int size);
static void add_status_response(EagleClient *client, int cmd, int status);
static void accept_common_handler(int fd);

static void set_response_header(ProtocolResponseHeader *header, uint8_t cmd, uint8_t status, uint32_t bodylen)
{
	header->magic = EG_PROTOCOL_RES;
	header->cmd = cmd;
	header->status = status;
	header->bodylen = bodylen;
}

static void set_event_header(ProtocolEventHeader *header, uint8_t cmd, uint8_t type, uint32_t bodylen)
{
	header->magic = EG_PROTOCOL_EVENT;
	header->cmd = cmd;
	header->type = type;
	header->bodylen = bodylen;
}

static void eject_queue_client(EagleClient *client)
{
	ListIterator iterator;
	ListNode *node;
	Queue_t *queue_t;

	list_rewind(client->subscribed_queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);
		unsubscribe_client_queue_t(queue_t, client);
	}

	list_rewind(client->declared_queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);

		undeclare_client_queue_t(queue_t, client);
		process_queue_t(queue_t);
	}
}

static void delete_users(void)
{
	ListIterator iterator;
	ListNode *node;
	EagleUser *user;

	list_rewind(server->users, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		user = EG_LIST_NODE_VALUE(node);

		if (BIT_CHECK(user->perm, EG_USER_NOT_CHANGE_PERM)) {
			continue;
		}

		list_delete_node(server->users, node);
	}
}

static void delete_queues(void)
{
	ListIterator iterator;
	ListNode *node;

	list_rewind(server->queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		list_delete_node(server->queues, node);
	}
}

static void auth_command_handler(EagleClient *client)
{
	ProtocolRequestAuth *req = (ProtocolRequestAuth*)client->request;
	EagleUser *user;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!check_input_buffer2(req->body.name, 32) || !check_input_buffer1(req->body.password, 32)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	user = find_user(server->users, req->body.name, req->body.password);
	if (!user) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_AUTH);
		return;
	}

	client->perm = user->perm;

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_AUTH);
}

static void ping_command_handler(EagleClient *client)
{
	ProtocolRequestPing *req = (ProtocolRequestPing*)client->request;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	add_status_response(client, req->cmd, EG_PROTOCOL_SUCCESS_PING);
}

static void stat_command_handler(EagleClient *client)
{
	ProtocolRequestStat *req = (ProtocolRequestStat*)client->request;
	ProtocolResponseStat *stat;
	struct rusage self_ru, c_ru;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	getrusage(RUSAGE_SELF, &self_ru);
	getrusage(RUSAGE_CHILDREN, &c_ru);

	stat = (ProtocolResponseStat*)xcalloc(sizeof(*stat));

	set_response_header(&stat->header, req->cmd, EG_PROTOCOL_SUCCESS_STAT, sizeof(stat->body));

	stat->body.version.major = EAGLE_VERSION_MAJOR;
	stat->body.version.minor = EAGLE_VERSION_MINOR;
	stat->body.version.patch = EAGLE_VERSION_PATCH;

	stat->body.uptime = time(NULL) - server->start_time;
	stat->body.used_cpu_sys = (float)self_ru.ru_stime.tv_sec + (float)self_ru.ru_stime.tv_usec/1000000;
	stat->body.used_cpu_user = (float)self_ru.ru_utime.tv_sec + (float)self_ru.ru_utime.tv_usec/1000000;
	stat->body.used_memory = xmalloc_used_memory();
	stat->body.used_memory_rss = xmalloc_memory_rss();
	stat->body.fragmentation_ratio = xmalloc_fragmentation_ratio();
	stat->body.clients = EG_LIST_LENGTH(server->clients);
	stat->body.users = EG_LIST_LENGTH(server->users);
	stat->body.queues = EG_LIST_LENGTH(server->queues);
	stat->body.resv1 = 0;
	stat->body.resv2 = 0;
	stat->body.resv3 = 0;
	stat->body.resv4 = 0;

	add_response(client, stat, sizeof(*stat));
}

static void save_comand_handler(EagleClient *client)
{
	ProtocolRequestSave *req = (ProtocolRequestSave*)client->request;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (req->body.async)
	{
		if (storage_save_background(server->storage) != EG_STATUS_OK) {
			add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_SAVE);
		}
	}
	else
	{
		if (storage_save(server->storage) != EG_STATUS_OK) {
			add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_SAVE);
		}
	}

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_SAVE);
}

static void flush_command_handler(EagleClient *client)
{
	ProtocolRequestFlush *req = (ProtocolRequestFlush*)client->request;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (BIT_CHECK(req->body.flags, EG_FLUSH_USER_FLAG)) {
		delete_users();
	}

	if (BIT_CHECK(req->body.flags, EG_FLUSH_QUEUE_FLAG)) {
		delete_queues();
	}

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_FLUSH);
}

static void disconnect_command_handler(EagleClient *client)
{
	ProtocolRequestDisconnect *req = (ProtocolRequestDisconnect*)client->request;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	free_client(client);
}

static void user_create_command_handler(EagleClient *client)
{
	ProtocolRequestUserCreate *req = (ProtocolRequestUserCreate*)client->request;
	EagleUser *user;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 32) || !check_input_buffer1(req->body.password, 32)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (find_user(server->users, req->body.name, NULL)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_CREATE);
		return;
	}

	user = create_user(req->body.name, req->body.password, req->body.perm);
	list_add_value_tail(server->users, user);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_USER_CREATE);
}

static void user_list_command_handler(EagleClient *client)
{
	ProtocolRequestUserList *req = (ProtocolRequestUserList*)client->request;
	ProtocolResponseHeader res;
	EagleUser *user;
	ListIterator iterator;
	ListNode *node;
	char *list;
	int i;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	set_response_header(&res, req->cmd, EG_PROTOCOL_SUCCESS_USER_LIST,
		EG_LIST_LENGTH(server->users) * (64 + sizeof(uint64_t)));

	list = (char*)xcalloc(sizeof(res) + res.bodylen);

	memcpy(list, &res, sizeof(res));

	i = sizeof(res);
	list_rewind(server->users, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		user = EG_LIST_NODE_VALUE(node);
		memcpy(list + i, user->name, strlenz(user->name));
		memcpy(list + i + 32, user->password, strlenz(user->password));
		memcpy(list + i + 64, &user->perm, sizeof(uint64_t));
		i += 64 + sizeof(uint64_t);
	}

	add_response(client, list, sizeof(res) + res.bodylen);
}

static void user_rename_command_handler(EagleClient *client)
{
	ProtocolRequestUserRename *req = (ProtocolRequestUserRename*)client->request;
	EagleUser *user;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.from, 32) || !check_input_buffer2(req->body.to, 32)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	user = find_user(server->users, req->body.from, NULL);
	if (!user || find_user(server->users, req->body.to, NULL)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_RENAME);
		return;
	}

	if (BIT_CHECK(user->perm, EG_USER_NOT_CHANGE_PERM)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_RENAME);
		return;
	}

	rename_user(user, req->body.to);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_USER_RENAME);
}

static void user_set_perm_command_handler(EagleClient *client)
{
	ProtocolRequestUserSetPerm *req = (ProtocolRequestUserSetPerm*)client->request;
	EagleUser *user;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 32)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	user = find_user(server->users, req->body.name, NULL);
	if (!user) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_SET_PERM);
		return;
	}

	if (BIT_CHECK(user->perm, EG_USER_NOT_CHANGE_PERM)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_SET_PERM);
		return;
	}

	set_user_perm(user, req->body.perm);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_USER_SET_PERM);
}

static void user_delete_command_handler(EagleClient *client)
{
	ProtocolRequestUserDelete *req = (ProtocolRequestUserDelete*)client->request;
	EagleUser *user;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 32)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	user = find_user(server->users, req->body.name, NULL);
	if (!user) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_DELETE);
		return;
	}

	if (BIT_CHECK(user->perm, EG_USER_NOT_CHANGE_PERM)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_DELETE);
		return;
	}

	if (list_delete_value(server->users, user) == EG_STATUS_ERR) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_USER_DELETE);
		return;
	}

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_USER_DELETE);
}

static void queue_create_command_handler(EagleClient *client)
{
	ProtocolRequestQueueCreate *req = (ProtocolRequestQueueCreate*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_CREATE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (find_queue_t(server->queues, req->body.name)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_CREATE);
		return;
	}

	if (req->body.max_msg_size > EG_MAX_MSG_SIZE) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_CREATE);
		return;
	}

	queue_t = create_queue_t(req->body.name, req->body.max_msg,
		((req->body.max_msg_size == 0) ? EG_MAX_MSG_SIZE : req->body.max_msg_size), req->body.flags);

	list_add_value_tail(server->queues, queue_t);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_CREATE);
}

static void queue_declare_command_handler(EagleClient *client)
{
	ProtocolRequestQueueDeclare *req = (ProtocolRequestQueueDeclare*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_DECLARE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(server->queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_DECLARE);
		return;
	}

	if (find_queue_t(client->declared_queues, req->body.name)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_DECLARE);
		return;
	}

	declare_client_queue_t(queue_t, client);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_DECLARE);
}

static void queue_exist_command_handler(EagleClient *client)
{
	ProtocolRequestQueueExist *req = (ProtocolRequestQueueExist*)client->request;
	ProtocolResponseQueueExist *res;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_EXIST_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	res = (ProtocolResponseQueueExist*)xcalloc(sizeof(*res));

	set_response_header(&res->header, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_EXIST, sizeof(res->body));

	if (find_queue_t(server->queues, req->body.name)) {
		res->body.status = 1;
	} else {
		res->body.status = 0;
	}

	add_response(client, res, sizeof(*res));
}

static void queue_list_command_handler(EagleClient *client)
{
	ProtocolRequestQueueList *req = (ProtocolRequestQueueList*)client->request;
	ProtocolResponseHeader res;
	ListIterator iterator;
	ListNode *node;
	Queue_t *queue_t;
	uint32_t queue_size, declared_clients, subscribed_clients;
	char *list;
	int i;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_LIST_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	set_response_header(&res, req->cmd, EG_PROTOCOL_SUCCESS_QUEUE_LIST,
		EG_LIST_LENGTH(server->queues) * (64 + (sizeof(uint32_t) * 6)));

	list = (char*)xcalloc(sizeof(res) + res.bodylen);

	memcpy(list, &res, sizeof(res));

	i = sizeof(res);
	list_rewind(server->queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);

		queue_size = get_size_queue_t(queue_t);
		declared_clients = get_declared_clients_queue_t(queue_t);
		subscribed_clients = get_subscribed_clients_queue_t(queue_t);

		memcpy(list + i, queue_t->name, strlenz(queue_t->name));
		i += 64;
		memcpy(list + i, &queue_t->max_msg, sizeof(uint32_t));
		i += sizeof(uint32_t);
		memcpy(list + i, &queue_t->max_msg_size, sizeof(uint32_t));
		i += sizeof(uint32_t);
		memcpy(list + i, &queue_t->flags, sizeof(uint32_t));
		i += sizeof(uint32_t);
		memcpy(list + i, &queue_size, sizeof(uint32_t));
		i += sizeof(uint32_t);
		memcpy(list + i, &declared_clients, sizeof(uint32_t));
		i += sizeof(uint32_t);
		memcpy(list + i, &subscribed_clients, sizeof(uint32_t));
		i += sizeof(uint32_t);
	}

	add_response(client, list, sizeof(res) + res.bodylen);
}

static void queue_size_command_handler(EagleClient *client)
{
	ProtocolRequestQueueSize *req = (ProtocolRequestQueueSize*)client->request;
	ProtocolResponseQueueSize *res;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_SIZE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(server->queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_SIZE);
		return;
	}

	res = (ProtocolResponseQueueSize*)xmalloc(sizeof(*res));

	set_response_header(&res->header, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_SIZE, sizeof(res->body));

	res->body.size = get_size_queue_t(queue_t);

	add_response(client, res, sizeof(*res));
}

static void queue_push_command_handler(EagleClient *client)
{
	ProtocolRequestHeader *req = (ProtocolRequestHeader*)client->request;
	Queue_t *queue_t;
	Object *msg;
	char *queue_name, *msg_data;
	int msg_size;

	if (client->pos < (sizeof(*req) + 65)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_PUSH_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	queue_name = client->request + sizeof(*req);

	if (!check_input_buffer2(queue_name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, queue_name);
	if (!queue_t) {
		add_status_response(client, req->cmd, EG_PROTOCOL_ERROR_QUEUE_PUSH);
		return;
	}

	msg_data = client->request + (sizeof(*req) + 64);
	msg_size = client->pos - (sizeof(*req) + 64);

	msg = create_dup_object(msg_data, msg_size);

	if (push_message_queue_t(queue_t, msg) == EG_STATUS_ERR) {
		add_status_response(client, req->cmd, EG_PROTOCOL_ERROR_QUEUE_PUSH);
		return;
	}

	add_status_response(client, req->cmd, EG_PROTOCOL_SUCCESS_QUEUE_PUSH);
}

static void queue_get_command_handler(EagleClient *client)
{
	ProtocolRequestQueueGet *req = (ProtocolRequestQueueGet*)client->request;
	ProtocolResponseHeader res;
	Queue_t *queue_t;
	Object *msg;
	char *buffer;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_GET_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_GET);
		return;
	}

	msg = get_message_queue_t(queue_t);
	if (!msg) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_GET);
		return;
	}

	set_response_header(&res, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_GET, OBJECT_SIZE(msg));

	buffer = (char*)xmalloc(sizeof(res) + OBJECT_SIZE(msg));

	memcpy(buffer, &res, sizeof(res));
	memcpy(buffer + sizeof(res), OBJECT_DATA(msg), OBJECT_SIZE(msg));

	add_response(client, buffer, sizeof(res) + res.bodylen);
}

static void queue_pop_command_handler(EagleClient *client)
{
	ProtocolRequestQueuePop *req = (ProtocolRequestQueuePop*)client->request;
	ProtocolResponseHeader res;
	Queue_t *queue_t;
	Object *msg;
	char *buffer;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_POP_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_POP);
		return;
	}

	msg = get_message_queue_t(queue_t);
	if (!msg) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_POP);
		return;
	}

	set_response_header(&res, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_POP, OBJECT_SIZE(msg));

	buffer = (char*)xmalloc(sizeof(res) + OBJECT_SIZE(msg));

	memcpy(buffer, &res, sizeof(res));
	memcpy(buffer + sizeof(res), OBJECT_DATA(msg), OBJECT_SIZE(msg));

	pop_message_queue_t(queue_t);

	add_response(client, buffer, sizeof(res) + res.bodylen);
}

static void queue_subscribe_command_handler(EagleClient *client)
{
	ProtocolRequestQueueSubscribe *req = (ProtocolRequestQueueSubscribe*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_SUBSCRIBE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_SUBSCRIBE);
		return;
	}

	if (find_queue_t(client->subscribed_queues, req->body.name)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_SUBSCRIBE);
		return;
	}

	subscribe_client_queue_t(queue_t, client, req->body.flags);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_SUBSCRIBE);
}

static void queue_unsubscribe_command_handler(EagleClient *client)
{
	ProtocolRequestQueueUnsubscribe *req = (ProtocolRequestQueueUnsubscribe*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_UNSUBSCRIBE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_UNSUBSCRIBE);
		return;
	}

	if (!find_queue_t(client->subscribed_queues, req->body.name)) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_UNSUBSCRIBE);
		return;
	}

	unsubscribe_client_queue_t(queue_t, client);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_UNSUBSCRIBE);
}

static void queue_purge_command_handler(EagleClient *client)
{
	ProtocolRequestQueuePurge *req = (ProtocolRequestQueuePurge*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_PURGE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(client->declared_queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_PURGE);
		return;
	}

	purge_queue_t(queue_t);

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_PURGE);
}

static void queue_delete_command_handler(EagleClient *client)
{
	ProtocolRequestQueueDelete *req = (ProtocolRequestQueueDelete*)client->request;
	Queue_t *queue_t;

	if (client->pos < sizeof(*req)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	if (!BIT_CHECK(client->perm, EG_USER_ADMIN_PERM) && !BIT_CHECK(client->perm, EG_USER_QUEUE_PERM)
		&& !BIT_CHECK(client->perm, EG_USER_QUEUE_DELETE_PERM)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_ACCESS);
		return;
	}

	if (!check_input_buffer2(req->body.name, 64)) {
		add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
		return;
	}

	queue_t = find_queue_t(server->queues, req->body.name);
	if (!queue_t) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_DELETE);
		return;
	}

	if (list_delete_value(server->queues, queue_t) == EG_STATUS_ERR) {
		add_status_response(client, req->header.cmd, EG_PROTOCOL_ERROR_QUEUE_DELETE);
		return;
	}

	add_status_response(client, req->header.cmd, EG_PROTOCOL_SUCCESS_QUEUE_DELETE);
}

void queue_client_event_notify(EagleClient *client, Queue_t *queue_t)
{
	ProtocolEventQueueNotify *event = (ProtocolEventQueueNotify*)xcalloc(sizeof(*event));

	set_event_header(&event->header, EG_PROTOCOL_CMD_QUEUE_SUBSCRIBE, EG_PROTOCOL_EVENT_NOTIFY, sizeof(event->body));

	memcpy(event->body.name, queue_t->name, strlenz(queue_t->name));

	add_response(client, event, sizeof(*event));
}

void queue_client_event_message(EagleClient *client, Queue_t *queue_t, Object *msg)
{
	ProtocolEventHeader header;
	char *buffer;

	set_event_header(&header, EG_PROTOCOL_CMD_QUEUE_SUBSCRIBE, EG_PROTOCOL_EVENT_MESSAGE, 64 + OBJECT_SIZE(msg));

	buffer = (char*)xcalloc(sizeof(header) + 64 + OBJECT_SIZE(msg));

	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + sizeof(header), queue_t->name, strlenz(queue_t->name));
	memcpy(buffer + sizeof(header) + 64, OBJECT_DATA(msg), OBJECT_SIZE(msg));

	add_response(client, buffer, sizeof(header) + 64 + OBJECT_SIZE(msg));
}

static void add_response(EagleClient *client, void *data, int size)
{
	Object *object = create_object(data, size);

	if (set_write_event(client) != EG_STATUS_OK) {
		release_object(object);
		return;
	}

	add_object_list(client->responses, object);
}

static void add_status_response(EagleClient *client, int cmd, int status)
{
	ProtocolResponseHeader *res;

	if (!client->noack)
	{
		res = (ProtocolResponseHeader*)xmalloc(sizeof(*res));

		set_response_header(res, cmd, status, 0);

		add_response(client, res, sizeof(*res));
	}
}

static inline void parse_command(EagleClient *client, ProtocolRequestHeader* req)
{
	client->noack = req->noack;

	switch (req->cmd)
	{
		case EG_PROTOCOL_CMD_AUTH:
			auth_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_PING:
			ping_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_STAT:
			stat_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_SAVE:
			save_comand_handler(client);
			break;

		case EG_PROTOCOL_CMD_FLUSH:
			flush_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_DISCONNECT:
			disconnect_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_USER_CREATE:
			user_create_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_USER_LIST:
			user_list_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_USER_RENAME:
			user_rename_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_USER_SET_PERM:
			user_set_perm_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_USER_DELETE:
			user_delete_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_CREATE:
			queue_create_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_DECLARE:
			queue_declare_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_EXIST:
			queue_exist_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_LIST:
			queue_list_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_SIZE:
			queue_size_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_PUSH:
			queue_push_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_GET:
			queue_get_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_POP:
			queue_pop_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_SUBSCRIBE:
			queue_subscribe_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_UNSUBSCRIBE:
			queue_unsubscribe_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_PURGE:
			queue_purge_command_handler(client);
			break;

		case EG_PROTOCOL_CMD_QUEUE_DELETE:
			queue_delete_command_handler(client);
			break;

		default:
			add_status_response(client, req->cmd, EG_PROTOCOL_ERROR_COMMAND);
			break;
	}
}

void process_request(EagleClient *client)
{
	ProtocolRequestHeader *req;

process:
	if (!client->bodylen)
	{
		if (client->nread < sizeof(*req)) {
			add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
			return;
		}

		req = (ProtocolRequestHeader*)(client->buffer + client->offset);

		if (req->magic != EG_PROTOCOL_REQ)
		{
			client->offset = 0;
			client->bodylen = 0;
			add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
			return;
		}

		if (req->bodylen > EG_MAX_BUF_SIZE)
		{
			client->offset = 0;
			client->bodylen = 0;
			add_status_response(client, 0, EG_PROTOCOL_ERROR_PACKET);
			return;
		}

		if (client->length < req->bodylen) {
			client->request = xrealloc(client->request, EG_MAX_BUF_SIZE);
			client->length = EG_MAX_BUF_SIZE;
		}

		int delta = client->nread - sizeof(*req);

		if (delta == (int)req->bodylen)
		{
			memcpy(client->request, client->buffer + client->offset, client->nread);
			client->pos = client->nread;
			client->offset = 0;
			client->nread = 0;
		}
		else if (delta < (int)req->bodylen)
		{
			memcpy(client->request, client->buffer + client->offset, client->nread);
			client->bodylen = req->bodylen - delta;
			client->pos = client->nread;
			return;
		}
		else if (delta > (int)req->bodylen)
		{
			client->pos = req->bodylen + sizeof(*req);
			memcpy(client->request, client->buffer + client->offset, client->pos);
			client->offset += client->pos;
			client->nread -= client->pos;
		}
	} else {
		if (client->nread == client->bodylen)
		{
			memcpy(client->request + client->pos, client->buffer, client->nread);
			client->pos += client->nread;
			client->offset = 0;
			client->nread = 0;
			client->bodylen = 0;
		}
		else if (client->nread < client->bodylen)
		{
			memcpy(client->request + client->pos, client->buffer, client->nread);
			client->pos += client->nread;
			client->bodylen -= client->nread;
			client->offset = 0;
			return;
		}
		else if (client->nread > client->bodylen)
		{
			memcpy(client->request + client->pos, client->buffer, client->bodylen);
			client->pos += client->bodylen;
			client->nread -= client->bodylen;
			client->offset = client->bodylen;
			client->bodylen = 0;
		}
	}

	req = (ProtocolRequestHeader*)client->request;
	parse_command(client, req);

	if ((int)client->nread >= (int)sizeof(*req)) {
		goto process;
	}
}

void read_request(EventLoop *loop, int fd, void *data, int mask)
{
	EagleClient *client = (EagleClient*)data;
	int nread;

	EG_NOTUSED(loop);
	EG_NOTUSED(mask);

	nread = read(fd, client->buffer, EG_BUF_SIZE);

	if (nread == -1) {
		if (errno == EAGAIN) {
			nread = 0;
		} else {
			free_client(client);
			return;
		}
	} else if (nread == 0) {
		free_client(client);
		return;
	}

	if (nread) {
		client->nread = nread;
		client->last_action = time(NULL);
	} else {
		return;
	}

	process_request(client);
}

static void send_response(EventLoop *loop, int fd, void *data, int mask)
{
	EagleClient *client = data;
	Object *object;
	int nwritten = 0, totwritten = 0;

	EG_NOTUSED(loop);
	EG_NOTUSED(mask);

	while (EG_LIST_LENGTH(client->responses))
	{
		object = EG_LIST_NODE_VALUE(EG_LIST_FIRST(client->responses));

		if (OBJECT_SIZE(object) == 0) {
			list_delete_node(client->responses, EG_LIST_FIRST(client->responses));
			continue;
		}

		nwritten = write(fd, ((char*)OBJECT_DATA(object)) + client->sentlen, OBJECT_SIZE(object) - client->sentlen);
		if (nwritten <= 0) {
			break;
		}

		client->sentlen += nwritten;
		totwritten += nwritten;

		if (client->sentlen == OBJECT_SIZE(object)) {
			list_delete_node(client->responses, EG_LIST_FIRST(client->responses));
			client->sentlen = 0;
		}
	}

	if (nwritten == -1) {
		if (errno == EAGAIN) {
			nwritten = 0;
		} else {
			free_client(client);
			return;
		}
	}

	if (totwritten > 0) {
		client->last_action = time(NULL);
	}

	if (EG_LIST_LENGTH(client->responses) == 0) {
		client->sentlen = 0;
		delete_file_event(server->loop, client->fd, EG_EVENT_WRITABLE);
	}
}

void client_timeout(void)
{
	EagleClient *client;
	ListIterator iterator;
	ListNode *node;

	if (server->client_timeout)
	{
		list_rewind(server->clients, &iterator);
		while ((node = list_next_node(&iterator)) != NULL)
		{
			client = EG_LIST_NODE_VALUE(node);
			if ((server->now_time - client->last_action) > server->client_timeout) {
				free_client(client);
			}
		}
	}
}

static int set_write_event(EagleClient *client)
{
	if (client->fd <= 0) {
		return EG_STATUS_ERR;
	}

	if (create_file_event(server->loop, client->fd, EG_EVENT_WRITABLE, send_response, client) == EG_EVENT_ERR &&
			EG_LIST_LENGTH(client->responses) == 0) {
		return EG_STATUS_ERR;
	}

    return EG_STATUS_OK;
}

EagleClient *create_client(int fd)
{
	EagleClient *client = (EagleClient*)xmalloc(sizeof(*client));

	net_set_nonblock(NULL, fd);
	net_tcp_nodelay(NULL, fd);

	if (create_file_event(server->loop, fd, EG_EVENT_READABLE, read_request, client) == EG_EVENT_ERR) {
		close(fd);
		xfree(client);
		return NULL;
	}

	client->fd = fd;
	client->perm = 0;
	client->request = (char*)xmalloc(EG_BUF_SIZE);
	client->length = EG_BUF_SIZE;
	client->pos = 0;
	client->noack = 0;
	client->offset = 0;
	client->buffer = (char*)xmalloc(EG_BUF_SIZE);
	client->bodylen = 0;
	client->nread = 0;
	client->responses = list_create();
	client->declared_queues = list_create();
	client->subscribed_queues = list_create();
	client->sentlen = 0;
	client->last_action = time(NULL);

	EG_LIST_SET_FREE_METHOD(client->responses, free_object_list_handler);

	list_add_value_tail(server->clients, client);

	return client;
}

void free_client(EagleClient *client)
{
	xfree(client->request);
	xfree(client->buffer);

	eject_queue_client(client);

	list_release(client->responses);
	list_release(client->declared_queues);
	list_release(client->subscribed_queues);

	delete_file_event(server->loop, client->fd, EG_EVENT_READABLE);
	delete_file_event(server->loop, client->fd, EG_EVENT_WRITABLE);

	close(client->fd);

	list_delete_value(server->clients, client);

	xfree(client);
}

void accept_tcp_handler(EventLoop *loop, int fd, void *data, int mask)
{
	int port, cfd;
	char ip[128];

	EG_NOTUSED(loop);
	EG_NOTUSED(mask);
	EG_NOTUSED(data);

	cfd = net_tcp_accept(server->error, fd, ip, &port);
	if (cfd == EG_NET_ERR) {
		warning("Error accept client: %s", server->error);
		return;
	}

	accept_common_handler(cfd);
}

void accept_unix_handler(EventLoop *loop, int fd, void *data, int mask)
{
	int cfd;

	EG_NOTUSED(loop);
	EG_NOTUSED(mask);
	EG_NOTUSED(data);

	cfd = net_unix_accept(server->error, fd);
	if (cfd == EG_NET_ERR) {
		warning("Accepting client connection: %s", server->error);
		return;
	}

	accept_common_handler(cfd);
}

static void accept_common_handler(int fd)
{
	EagleClient *client;

	if ((client = create_client(fd)) == NULL) {
		close(fd);
		return;
	}

	if (EG_LIST_LENGTH(server->clients) > server->max_clients) {
		free_client(client);
	}
}

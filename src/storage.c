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

#include "fmacros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "eagle.h"
#include "storage.h"
#include "version.h"
#include "list.h"
#include "user.h"
#include "queue_t.h"
#include "queue.h"
#include "lzf.h"
#include "xmalloc.h"
#include "utils.h"

static int storage_write(FILE *fp, void *buffer, size_t length)
{
	if (fwrite(buffer, length, 1, fp) == 0)
		return -1;

	return length;
}

static int storage_read(FILE *fp, void *buffer, size_t length)
{
	if (fread(buffer, length, 1, fp) == 0)
		return -1;

	return length;
}

static int storage_write_type(FILE *fp, unsigned char type)
{
	return storage_write(fp, &type, 1);
}

static int storage_read_type(FILE *fp)
{
	unsigned char type;

	if (storage_read(fp, &type, 1) == -1)
		return -1;

	return type;
}

static int storage_write_magic(FILE *fp)
{
	char magic[16];

	snprintf(magic, sizeof(magic), "EagleMQ%04d", EG_STORAGE_VERSION);
	if (storage_write(fp, magic, 11) == -1)
		return EG_STATUS_ERR;

	return EG_STATUS_OK;
}

static int storage_read_magic(FILE *fp)
{
	char magic[16];
	int version;

	if (storage_read(fp, magic, 11) == -1)
		return EG_STATUS_ERR;

	magic[11] = '\0';

	if (memcmp(magic, "EagleMQ", 7))
	{
		warning("Bad storage signature");
		return EG_STATUS_ERR;
	}

	version = atoi(magic + 7);
	if (version < 1 || version > EG_STORAGE_VERSION) {
		warning("Bad storage version");
		return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;
}

static int storage_save_user(FILE *fp, EagleUser *user)
{
	uint32_t name_length = strlenz(user->name);
	uint32_t password_length = strlenz(user->password);

	if (storage_write_type(fp, EG_STORAGE_TYPE_USER) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &name_length, sizeof(name_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, user->name, name_length) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &password_length, sizeof(password_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, user->password, password_length) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &user->perm, sizeof(user->perm)) == -1)
		return EG_STATUS_ERR;

	return EG_STATUS_OK;
}

static int storage_save_users(FILE *fp)
{
	ListIterator iterator;
	ListNode *node;
	EagleUser *user;

	list_rewind(server->users, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		user = EG_LIST_NODE_VALUE(node);

		if (BIT_CHECK(user->perm, EG_USER_NOT_CHANGE_PERM))
			continue;

		if (storage_save_user(fp, user) != EG_STATUS_OK)
			return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;
}

static int storage_save_queue_message(FILE *fp, Object *msg)
{
	uint32_t message_length = msg->size;

	if (storage_write_type(fp, EG_STORAGE_TYPE_MESSAGE) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &message_length, sizeof(message_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, msg->data, message_length) == -1)
		return EG_STATUS_ERR;

	return EG_STATUS_OK;
}

static int storage_save_queue_messages(FILE *fp, Queue_t *queue_t)
{
	QueueIterator *iterator;
	QueueNode *node;
	Object *msg;

	iterator = queue_get_iterator(queue_t->queue, EG_START_TAIL);

	while ((node = queue_next_node(iterator)) != NULL)
	{
		msg = EG_QUEUE_NODE_VALUE(node);

		if (storage_save_queue_message(fp, msg) != EG_STATUS_OK) {
			queue_release_iterator(iterator);
			return EG_STATUS_ERR;
		}
	}

	queue_release_iterator(iterator);

	return EG_STATUS_OK;
}

static int storage_save_queue(FILE *fp, Queue_t *queue_t)
{
	uint32_t name_length = strlenz(queue_t->name);
	uint32_t queue_size = get_size_queue_t(queue_t);

	if (storage_write_type(fp, EG_STORAGE_TYPE_QUEUE) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &name_length, sizeof(name_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, queue_t->name, name_length) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &queue_t->max_msg, sizeof(queue_t->max_msg)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &queue_t->max_msg_size, sizeof(queue_t->max_msg_size)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &queue_t->flags, sizeof(queue_t->flags)) == -1)
		return EG_STATUS_ERR;

	if (storage_write(fp, &queue_size, sizeof(queue_size)) == -1)
		return EG_STATUS_ERR;

	return storage_save_queue_messages(fp, queue_t);
}

static int storage_save_queues(FILE *fp)
{
	ListIterator iterator;
	ListNode *node;
	Queue_t *queue_t;

	list_rewind(server->queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);

		if (!BIT_CHECK(queue_t->flags, EG_QUEUE_DURABLE_FLAG))
			continue;

		if (storage_save_queue(fp, queue_t) != EG_STATUS_OK)
			return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;
}

static int storage_load_user(FILE *fp)
{
	EagleUser user;
	uint32_t name_length;
	uint32_t password_length;

	if (storage_read(fp, &name_length, sizeof(name_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &user.name, name_length) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &password_length, sizeof(password_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &user.password, password_length) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &user.perm, sizeof(user.perm)) == -1)
		return EG_STATUS_ERR;

	list_add_value_tail(server->users, create_user(user.name, user.password, user.perm));

	return EG_STATUS_OK;
}

static int storage_load_queue_message(FILE *fp, Queue_t *queue_t)
{
	uint32_t message_length;
	void *data;

	if (storage_read_type(fp) != EG_STORAGE_TYPE_MESSAGE)
		return EG_STATUS_ERR;

	if (storage_read(fp, &message_length, sizeof(message_length)) == -1)
		return EG_STATUS_ERR;

	data = xmalloc(message_length);

	if (storage_read(fp, data, message_length) == -1)
		return EG_STATUS_ERR;

	push_message_queue_t(queue_t, create_object(data, message_length));

	return EG_STATUS_OK;
}

static int storage_load_queue(FILE *fp)
{
	Queue_t *queue_t;
	Queue_t data;
	uint32_t name_length;
	uint32_t queue_size;
	int i;

	if (storage_read(fp, &name_length, sizeof(name_length)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &data.name, name_length) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &data.max_msg, sizeof(data.max_msg)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &data.max_msg_size, sizeof(data.max_msg_size)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &data.flags, sizeof(data.flags)) == -1)
		return EG_STATUS_ERR;

	if (storage_read(fp, &queue_size, sizeof(queue_size)) == -1)
		return EG_STATUS_ERR;

	queue_t = create_queue_t(data.name, data.max_msg, data.max_msg_size, data.flags);

	for (i = 0; i < queue_size; i++)
	{
		if (storage_load_queue_message(fp, queue_t) != EG_STATUS_OK)
			return EG_STATUS_ERR;
	}

	list_add_value_tail(server->queues, queue_t);

	return EG_STATUS_OK;
}

int storage_load(char *filename)
{
	FILE *fp;
	int type;
	int state = 1;

	fp = fopen(filename, "r");
	if (!fp) {
		warning("Failed opening storage for loading: %s",  strerror(errno));
		return EG_STATUS_ERR;
	}

	if (storage_read_magic(fp) != EG_STATUS_OK)
		goto error;

	while (state)
	{
		if ((type = storage_read_type(fp)) == -1)
			goto error;

		switch (type)
		{
			case EG_STORAGE_TYPE_USER:
				if (storage_load_user(fp) != EG_STATUS_OK) goto error;
				break;

			case EG_STORAGE_TYPE_QUEUE:
				if (storage_load_queue(fp) != EG_STATUS_OK) goto error;
				break;

			case EG_STORAGE_EOF:
				state = 0;
				break;

			default:
				goto error;
		}
	}

	fclose(fp);

	return EG_STATUS_OK;

error:
	fclose(fp);

	fatal("Error read storage %s", filename);

	return EG_STATUS_ERR;
}

int storage_save(char *filename)
{
	FILE *fp;
	char tmpfile[32];

	snprintf(tmpfile, sizeof(tmpfile), "eaglemq-%d.dat", (int)getpid());

	fp = fopen(tmpfile, "w");
	if (!fp) {
		warning("Failed opening storage for saving: %s",  strerror(errno));
		return EG_STATUS_ERR;
	}

	if (storage_write_magic(fp) != EG_STATUS_OK)
		goto error;

	if (storage_save_users(fp) != EG_STATUS_OK)
		goto error;

	if (storage_save_queues(fp) != EG_STATUS_OK)
		goto error;

	if (storage_write_type(fp, EG_STORAGE_EOF) == -1)
		goto error;

	fflush(fp);
	fsync(fileno(fp));
	fclose(fp);

	if (rename(tmpfile, filename) == -1)
	{
		warning("Error rename temp storage file %s to %s: %s", tmpfile, filename, strerror(errno));
		unlink(tmpfile);
		return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;

error:
	warning("Error write data to the storage");

	fclose(fp);
	unlink(tmpfile);

	return EG_STATUS_ERR;
}

int storage_save_background(char *filename)
{
	pid_t child_pid;

	if (server->child_pid != -1)
		return EG_STATUS_ERR;

	if ((child_pid = fork()) == 0)
	{
		if (server->fd > 0)
			close(server->fd);

		if (server->sfd > 0)
			close(server->sfd);

		_exit(storage_save(filename));
	}
	else
	{
		if (child_pid == -1)
		{
			warning("Error save data in background: %s\n", strerror(errno));
			return EG_STATUS_ERR;
		}

		server->child_pid = child_pid;
	}

	return EG_STATUS_OK;
}

void remove_temp_file(pid_t pid)
{
	char tmpfile[32];

	snprintf(tmpfile, sizeof(tmpfile), "eaglemq-%d.dat", (int)pid);
	unlink(tmpfile);
}

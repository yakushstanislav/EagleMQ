/*
   Copyright (c) 2012, Yakush Stanislav(st.yakush@yandex.ru)
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

#ifndef __QUEUE_T_LIB_H__
#define __QUEUE_T_LIB_H__

#include <stdint.h>

#include "eagle.h"
#include "queue.h"
#include "list.h"
#include "object.h"

#define EG_QUEUE_AUTODELETE_BIT 0

#define EG_QUEUE_ASYNC_GET_FLAG 0
#define EG_QUEUE_ASYNC_POP_FLAG 1

typedef struct QueueAsyncClient {
	EagleClient *client;
	uint32_t flags;
} QueueClient;

typedef struct Queue_t
{
	char name[64];
	uint32_t max_msg;
	uint32_t max_msg_size;
	uint32_t flags;
	List *declared_clients;
	List *subscribed_clients;
	Queue *queue;
	void (*declare_client)(void *queue_ptr, void *client_ptr);
	int (*undeclare_client)(void *queue_ptr, void *client_ptr);
	void (*subscribe_client)(void *queue_ptr, void *client_ptr, uint32_t flags);
	int (*unsubscribe_client)(void *queue_ptr, void *client_ptr);
} Queue_t;

#define EG_QUEUE_SET_DECLARE_METHOD(q, m) ((q)->declare_client = (m))
#define EG_QUEUE_SET_UNDECLARE_METHOD(q, m) ((q)->undeclare_client = (m))

#define EG_QUEUE_SET_SUBSCRIBE_METHOD(q, m) ((q)->subscribe_client = (m))
#define EG_QUEUE_SET_UNSUBSCRIBE_METHOD(q, m) ((q)->unsubscribe_client = (m))

Queue_t *create_queue_t(const char *name, uint32_t max_msg, uint32_t max_msg_size, uint32_t flags);
void delete_queue_t(Queue_t *queue_t);
int push_value_queue_t(Queue_t *queue_t, Object *value);
Object *get_value_queue_t(Queue_t *queue_t);
void pop_value_queue_t(Queue_t *queue_t);
Queue_t *find_queue_t(List *list, const char *name);
uint32_t get_size_queue_t(Queue_t *queue_t);
void purge_queue_t(Queue_t *queue_t);
void declare_queue_client(Queue_t *queue_t, void *client);
int undeclare_queue_client(Queue_t *queue_t, void *client);
void subscribe_queue_client(Queue_t *queue_t, void *client, uint32_t flags);
int unsubscribe_queue_client(Queue_t *queue_t, void *client);
void eject_queue_clients(Queue_t *queue_t);
void add_queue_list(List *list, Queue_t *queue_t);
int delete_queue_list(List *list, Queue_t *queue_t);
void free_queue_list_handler(void *ptr);

#endif

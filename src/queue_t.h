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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdint.h>

#include "eagle.h"
#include "queue.h"
#include "list.h"
#include "object.h"

#define EG_QUEUE_AUTODELETE_FLAG 0
#define EG_QUEUE_FORCE_PUSH_FLAG 1

#define EG_QUEUE_CLIENT_NOTIFY_FLAG 0

typedef struct QueueAsyncClient {
	EagleClient *client;
	uint32_t flags;
} QueueClient;

typedef struct Queue_t {
	char name[64];
	uint32_t max_msg;
	uint32_t max_msg_size;
	uint32_t flags;
	int auto_delete;
	int force_push;
	Queue *queue;
	List *declared_clients;
	List *subscribed_clients;
} Queue_t;

Queue_t *create_queue_t(const char *name, uint32_t max_msg, uint32_t max_msg_size, uint32_t flags);
void delete_queue_t(Queue_t *queue_t);
int push_message_queue_t(Queue_t *queue_t, Object *msg);
Object *get_message_queue_t(Queue_t *queue_t);
void pop_message_queue_t(Queue_t *queue_t);
Queue_t *find_queue_t(List *list, const char *name);
uint32_t get_declared_clients_queue_t(Queue_t *queue_t);
uint32_t get_subscribed_clients_queue_t(Queue_t *queue_t);
uint32_t get_size_queue_t(Queue_t *queue_t);
void purge_queue_t(Queue_t *queue_t);
void declare_client_queue_t(Queue_t *queue_t, EagleClient *client);
void undeclare_client_queue_t(Queue_t *queue_t, EagleClient *client);
void subscribe_client_queue_t(Queue_t *queue_t, EagleClient *client, uint32_t flags);
void unsubscribe_client_queue_t(Queue_t *queue_t, EagleClient *client);
int process_queue_t(Queue_t *queue_t);
void eject_clients_queue_t(Queue_t *queue_t);
void add_queue_list(List *list, Queue_t *queue_t);
int delete_queue_list(List *list, Queue_t *queue_t);
void free_queue_list_handler(void *ptr);

#endif

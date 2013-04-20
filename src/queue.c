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

#include "queue.h"
#include "xmalloc.h"

Queue *queue_create(void)
{
	Queue *queue;

	queue = (Queue*)xmalloc(sizeof(*queue));

	queue->head = queue->tail = NULL;
	queue->len = 0;
	queue->free = NULL;

	return queue;
}

void queue_release(Queue *queue)
{
	unsigned int len;
	QueueNode *current, *next;

	current = queue->head;
	len = queue->len;

	while (len--) {
		next = current->next;

		if (queue->free) {
			queue->free(current->value);
		}

		xfree(current);
		current = next;
	}

    xfree(queue);
}

Queue *queue_push_value_head(Queue *queue, void *value)
{
	QueueNode *node;

	node = (QueueNode*)xmalloc(sizeof(*node));

	node->value = value;

	if (queue->len == 0) {
		queue->head = queue->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = NULL;
		node->next = queue->head;
		queue->head->prev = node;
		queue->head = node;
	}

	queue->len++;

	return queue;
}

Queue *queue_push_value_tail(Queue *queue, void *value)
{
	QueueNode *node;

	node = (QueueNode*)xmalloc(sizeof(*node));

	node->value = value;

	if (queue->len == 0) {
		queue->head = queue->tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = queue->tail;
		node->next = NULL;
		queue->tail->next = node;
		queue->tail = node;
	}

	queue->len++;

	return queue;
}

void *queue_get_value(Queue *queue)
{
	void *value;

	if (!queue->len) {
		return NULL;
	}

	value = EG_QUEUE_NODE_VALUE(queue->tail);

	return value;
}

void *queue_pop_value(Queue *queue)
{
	QueueNode *prev;
	void *value;

	if (!queue->len) {
		return NULL;
	}

	value = EG_QUEUE_NODE_VALUE(queue->tail);
	prev = queue->tail->prev;

	xfree(queue->tail);

	queue->tail = prev;
	queue->len--;

	return value;
}

Queue *queue_purge(Queue *queue)
{
	unsigned int len;
	QueueNode *current, *next;

	current = queue->head;
	len = queue->len;

	while (len--) {
		next = current->next;

		if (queue->free) {
			queue->free(current->value);
		}

		xfree(current);
		current = next;
	}

	queue->len = 0;

	return queue;
}

void queue_delete_node(Queue *queue, QueueNode *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		queue->head = node->next;
	}

	if (node->next) {
		node->next->prev = node->prev;
	} else {
		queue->tail = node->prev;
	}

	if (queue->free) {
		queue->free(node->value);
	}

	xfree(node);
	queue->len--;
}

QueueIterator *queue_get_iterator(Queue *queue, int direction)
{
	QueueIterator *iter;

	iter = (QueueIterator*)xmalloc(sizeof(*iter));

	if (direction == EG_START_HEAD) {
		iter->next = queue->head;
	} else {
		iter->next = queue->tail;
	}

	iter->direction = direction;

	return iter;
}

void queue_release_iterator(QueueIterator *iter)
{
	xfree(iter);
}

QueueNode *queue_next_node(QueueIterator *iter)
{
	QueueNode *current = iter->next;

	if (current != NULL) {
		if (iter->direction == EG_START_HEAD) {
			iter->next = current->next;
		} else {
			iter->next = current->prev;
		}
	}

	return current;
}

void queue_rewind(Queue *queue, QueueIterator *iter)
{
	iter->next = queue->head;
	iter->direction = EG_START_HEAD;
}

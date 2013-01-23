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

#ifndef __QUEUE_LIB_H__
#define __QUEUE_LIB_H__

#define EG_START_HEAD 0
#define EG_START_TAIL 1

#define EG_QUEUE_LENGTH(q) ((q)->len)
#define EG_QUEUE_FIRST(q) ((q)->head)
#define EG_QUEUE_LAST(q) ((q)->tail)
#define EG_QUEUE_PREV_NODE(n) ((n)->prev)
#define EG_QUEUE_NEXT_NODE(n) ((n)->next)
#define EG_QUEUE_NODE_VALUE(n) ((n)->value)

#define EG_QUEUE_SET_FREE_METHOD(q, m) ((q)->free = (m))
#define EG_QUEUE_GET_FREE_METHOD(q) ((q)->free)

typedef struct QueueNode {
	struct QueueNode *prev;
	struct QueueNode *next;
	void *value;
} QueueNode;

typedef struct QueueIterator {
  QueueNode *next;
  int direction;
} QueueIterator;

typedef struct Queue {
	QueueNode *head;
	QueueNode *tail;
	void (*free)(void *ptr);
	unsigned int len;
} Queue;

Queue *queue_create(void);
void queue_release(Queue *queue);
Queue *queue_push_value(Queue *queue, void *value);
void *queue_get_value(Queue *queue);
void *queue_pop_value(Queue *queue);
Queue *queue_purge(Queue *queue);
QueueIterator *queue_get_iterator(Queue *queue, int direction);
void queue_release_iterator(QueueIterator *iter);
QueueNode *queue_next_node(QueueIterator *iter);
void queue_rewind(Queue *queue, QueueIterator *iter);

#endif

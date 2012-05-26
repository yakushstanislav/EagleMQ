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

#ifndef __EVENT_LIB_H__
#define __EVENT_LIB_H__

#define EG_EVENT_OK 0
#define EG_EVENT_ERR -1

#define EG_EVENT_NONE 0
#define EG_EVENT_READABLE 1
#define EG_EVENT_WRITEABLE 2

#define EG_EVENT_FILE 1
#define EG_EVENT_TIME 2
#define EG_EVENTS_ALL (EG_EVENT_FILE|EG_EVENT_TIME)
#define EG_DONT_WAIT 4

struct EventLoop;

typedef void file_handler(struct EventLoop *loop, int fd, void *data, int mask);
typedef int time_handler(struct EventLoop *loop, long long id, void *data);
typedef void finalizer_handler(struct EventLoop *loop, void *data);
typedef void before_sleep_handler(struct EventLoop *loop);

typedef struct FileEvent
{
	int mask;
	file_handler *read_handler;
	file_handler *write_handler;
	void *data;
} FileEvent;

typedef struct TimeEvent
{
	long long id;
	long sec;
	long ms;
	time_handler *time_handler;
	finalizer_handler *finalizer_handler;
	void *data;
	struct TimeEvent *next;
} TimeEvent;

typedef struct FiredEvent
{
	int fd;
	int mask;
} FiredEvent;

typedef struct EventLoop
{
	int maxfd;
	int size;
	long long time_event_id;
	FileEvent *events;
	FiredEvent *fired;
	TimeEvent *time_ev;
	int stop;
	void *api_data;
	before_sleep_handler *before_sleep_handler;
} EventLoop;

EventLoop *create_event_loop(int size);
void delete_event_loop(EventLoop *loop);
void stop_event_loop(EventLoop *loop);
int create_file_event(EventLoop *loop, int fd, int mask, file_handler *handler, void *data);
void delete_file_event(EventLoop *loop, int fd, int mask);
int get_file_events(EventLoop *loop, int fd);
long long create_time_event(EventLoop *loop, long long milliseconds, time_handler *time_handler, finalizer_handler *finalizer_handler, void *data);
int delete_time_event(EventLoop *loop, long long id);
int process_events(EventLoop *loop, int flags);
void start_main_loop(EventLoop *loop);
void set_before_sleep_handler(EventLoop *loop, before_sleep_handler *before_sleep_handler);
char *get_event_api_name(void);

#endif

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

/* Taken from Redis */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include <string.h>

#include "xmalloc.h"
#include "event.h"

#ifdef _EVENT_SELECT_
	#include "event_select.c"
#else
	#include "event_epoll.c"
#endif

static void get_time(long *seconds, long *milliseconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	*seconds = tv.tv_sec;
	*milliseconds = tv.tv_usec / 1000;
}

static void add_milliseconds_to_now(long long milliseconds, long *sec, long *ms)
{
	long cur_sec, cur_ms, when_sec, when_ms;

	get_time(&cur_sec, &cur_ms);

	when_sec = cur_sec + milliseconds / 1000;
	when_ms = cur_ms + milliseconds % 1000;

	if (when_ms >= 1000) {
		when_sec++;
		when_ms -= 1000;
	}

	*sec = when_sec;
	*ms = when_ms;
}

static TimeEvent *search_nearest_timer(EventLoop *loop)
{
	TimeEvent *time_event = loop->time_event;
	TimeEvent *nearest = NULL;

	while (time_event)
	{
		if (!nearest || time_event->sec < nearest->sec ||
			(time_event->sec == nearest->sec && time_event->ms < nearest->ms)) {
			nearest = time_event;
		}

		time_event = time_event->next;
	}

	return nearest;
}

static int process_time_events(EventLoop *loop)
{
	int processed = 0;
	TimeEvent *time_event = loop->time_event;
	long long max_id = loop->time_event_id - 1;
	long sec, ms;
	long long id;
	int retval;

	while (time_event)
	{
		if (time_event->id > max_id) {
			time_event = time_event->next;
			continue;
		}

		get_time(&sec, &ms);

		if (sec > time_event->sec || (sec == time_event->sec && ms >= time_event->ms))
		{
			id = time_event->id;
			retval = time_event->time_handler(loop, id, time_event->data);
			processed++;

			if (retval != -1) {
				add_milliseconds_to_now(retval, &time_event->sec, &time_event->ms);
			} else {
				delete_time_event(loop, id);
			}

			time_event = loop->time_event;
		} else {
			time_event = time_event->next;
		}
	}

	return processed;
}

EventLoop *create_event_loop(int size)
{
	EventLoop *loop;
	int i;

	loop = (EventLoop*)xmalloc(sizeof(*loop));

	loop->events = (FileEvent*)xmalloc(sizeof(FileEvent) * size);
	loop->fired = (FiredEvent*)xmalloc(sizeof(FiredEvent) * size);

	loop->maxfd = -1;
	loop->size = size;
	loop->time_event_id = 0;
	loop->time_event = NULL;
	loop->stop = 0;
	loop->before_sleep_handler = NULL;

	if (event_api_init(loop) == -1) {
		xfree(loop->events);
		xfree(loop->fired);
		xfree(loop);
		return NULL;
	}

	for (i = 0; i < size; i++) {
		loop->events[i].mask = EG_EVENT_NONE;
	}

	return loop;
}

void delete_event_loop(EventLoop *loop)
{
	event_api_free(loop);
	xfree(loop->events);
	xfree(loop->fired);
	xfree(loop);
}

void stop_event_loop(EventLoop *loop)
{
	loop->stop = 1;
}

int create_file_event(EventLoop *loop, int fd, int mask, file_handler *handler, void *data)
{
	FileEvent *file_event;

	if (fd >= loop->size) {
		return EG_EVENT_ERR;
	}

	file_event = &loop->events[fd];

	if (event_api_add(loop, fd, mask) == -1) {
		return EG_EVENT_ERR;
	}

	file_event->mask |= mask;

	if (mask & EG_EVENT_READABLE) {
		file_event->read_handler = handler;
	}

	if (mask & EG_EVENT_WRITABLE) {
		file_event->write_handler = handler;
	}

	file_event->data = data;

	if (fd > loop->maxfd) {
		loop->maxfd = fd;
	}

	return EG_EVENT_OK;
}

void delete_file_event(EventLoop *loop, int fd, int mask)
{
	FileEvent *file_event;
	int i;

	if (fd >= loop->size) {
		return;
	}

	file_event = &loop->events[fd];

	if (file_event->mask == EG_EVENT_NONE) {
		return;
	}

	file_event->mask = file_event->mask & (~mask);

	if (fd == loop->maxfd && file_event->mask == EG_EVENT_NONE) {
		for (i = loop->maxfd - 1; i >= 0; i--) {
			if (loop->events[i].mask != EG_EVENT_NONE) {
				break;
			}
		}
		loop->maxfd = i;
	}

	event_api_delete(loop, fd, mask);
}

int get_file_events(EventLoop *loop, int fd)
{
	FileEvent *file_event;

	if (fd >= loop->size) {
		return 0;
	}

	file_event = &loop->events[fd];

	return file_event->mask;
}

long long create_time_event(EventLoop *loop, long long milliseconds, time_handler *time_handler,
	finalizer_handler *finalizer_handler, void *data)
{
	TimeEvent *time_event = (TimeEvent*)xmalloc(sizeof(*time_event));
	long long id = loop->time_event_id++;

	time_event->id = id;

	add_milliseconds_to_now(milliseconds, &time_event->sec, &time_event->ms);

	time_event->time_handler = time_handler;
	time_event->finalizer_handler = finalizer_handler;
	time_event->data = data;
	time_event->next = loop->time_event;

	loop->time_event = time_event;

	return id;
}

int delete_time_event(EventLoop *loop, long long id)
{
	TimeEvent *time_event = loop->time_event, *prev = NULL;

	while (time_event)
	{
		if (time_event->id == id)
		{
			if (prev == NULL) {
				loop->time_event = time_event->next;
			} else {
				prev->next = time_event->next;
			}

			if (time_event->finalizer_handler) {
				time_event->finalizer_handler(loop, time_event->data);
			}

			xfree(time_event);
			return EG_EVENT_OK;
		}

		prev = time_event;
		time_event = time_event->next;
	}

	return EG_EVENT_ERR;
}

int process_events(EventLoop *loop, int flags)
{
	FileEvent *file_event;
	TimeEvent *shortest = NULL;
	struct timeval tv, *tvp;
	int processed = 0, i, mask, fd, rfired, events;
	long sec, ms;

	if (!(flags & EG_EVENT_TIME) && !(flags & EG_EVENT_FILE)) {
		return 0;
	}

	if (loop->maxfd != -1 || ((flags & EG_EVENT_TIME) && !(flags & EG_DONT_WAIT)))
	{
		if (flags & EG_EVENT_TIME && !(flags & EG_DONT_WAIT)) {
			shortest = search_nearest_timer(loop);
		}

		if (shortest)
		{
			get_time(&sec, &ms);

			tvp = &tv;
			tvp->tv_sec = shortest->sec - sec;

			if (shortest->ms < ms) {
				tvp->tv_usec = ((shortest->ms + 1000) - ms) * 1000;
				tvp->tv_sec--;
			} else {
				tvp->tv_usec = (shortest->ms - ms) * 1000;
			}

			if (tvp->tv_sec < 0) {
				tvp->tv_sec = 0;
			}

			if (tvp->tv_usec < 0) {
				tvp->tv_usec = 0;
			}
		} else {
			if (flags & EG_DONT_WAIT) {
				tv.tv_sec = tv.tv_usec = 0;
				tvp = &tv;
			} else {
				tvp = NULL;
			}
		}

		events = event_api_poll(loop, tvp);
		for (i = 0; i < events; i++)
		{
			file_event = &loop->events[loop->fired[i].fd];
			mask = loop->fired[i].mask;
			fd = loop->fired[i].fd;
			rfired = 0;

			if (file_event->mask & mask & EG_EVENT_READABLE) {
				rfired = 1;
				file_event->read_handler(loop, fd, file_event->data, mask);
			}

			if (file_event->mask & mask & EG_EVENT_WRITABLE) {
				if (!rfired || file_event->write_handler != file_event->read_handler) {
					file_event->write_handler(loop, fd, file_event->data, mask);
				}
			}
			processed++;
		}
	}

	if (flags & EG_EVENT_TIME) {
		processed += process_time_events(loop);
	}

	return processed;
}

int wait(int fd, int mask, long long milliseconds)
{
	struct pollfd pfd;
	int retmask = 0, retval;

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;

	if (mask & EG_EVENT_READABLE)
		pfd.events |= POLLIN;

	if (mask & EG_EVENT_WRITABLE)
		pfd.events |= POLLOUT;

	if ((retval = poll(&pfd, 1, milliseconds)) == 1)
	{
		if (pfd.revents & POLLIN)
			retmask |= EG_EVENT_READABLE;

		if (pfd.revents & POLLOUT)
			retmask |= EG_EVENT_WRITABLE;

		if (pfd.revents & POLLERR)
			retmask |= EG_EVENT_WRITABLE;

		if (pfd.revents & POLLHUP)
			retmask |= EG_EVENT_WRITABLE;

		return retmask;
	} else {
		return retval;
	}
}

void start_main_loop(EventLoop *loop)
{
	loop->stop = 0;

	while (!loop->stop) {
		if (loop->before_sleep_handler != NULL) {
			loop->before_sleep_handler(loop);
		}
		process_events(loop, EG_EVENTS_ALL);
	}
}

void set_before_sleep_handler(EventLoop *loop, before_sleep_handler *before_sleep_handler)
{
	loop->before_sleep_handler = before_sleep_handler;
}

char *get_event_api_name(void)
{
	return event_api_name();
}

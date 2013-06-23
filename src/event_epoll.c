/*
   Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Redis nor the
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

#include <sys/epoll.h>

typedef struct ApiData {
	int epfd;
	struct epoll_event *events;
} ApiData;

static int event_api_init(EventLoop *loop)
{
	ApiData *data = (ApiData*)xmalloc(sizeof(*data));

	data->events = (struct epoll_event*)xmalloc(sizeof(struct epoll_event) * loop->size);

	data->epfd = epoll_create(1024);
	if (data->epfd == -1) {
		xfree(data->events);
		xfree(data);
		return -1;
	}

	loop->api_data = data;

	return 0;
}

static void event_api_free(EventLoop *loop)
{
	ApiData *data = (ApiData*)loop->api_data;

	close(data->epfd);
	xfree(data->events);
	xfree(data);
}

static int event_api_add(EventLoop *loop, int fd, int mask)
{
	ApiData *data = (ApiData*)loop->api_data;
	struct epoll_event epoll_ev;
	int op = loop->events[fd].mask == EG_EVENT_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	epoll_ev.events = 0;
	mask |= loop->events[fd].mask;

	if (mask & EG_EVENT_READABLE) {
		epoll_ev.events |= EPOLLIN;
	}

	if (mask & EG_EVENT_WRITABLE) {
		epoll_ev.events |= EPOLLOUT;
	}

	epoll_ev.data.u64 = 0;
	epoll_ev.data.fd = fd;

	if (epoll_ctl(data->epfd, op, fd, &epoll_ev) == -1) {
		return -1;
	}

	return 0;
}

static void event_api_delete(EventLoop *loop, int fd, int mask)
{
	ApiData *data = (ApiData*)loop->api_data;
	struct epoll_event epoll_ev;
	int nmask = loop->events[fd].mask & (~mask);

	epoll_ev.events = 0;

	if (nmask & EG_EVENT_READABLE) {
		epoll_ev.events |= EPOLLIN;
	}

	if (nmask & EG_EVENT_WRITABLE) {
		epoll_ev.events |= EPOLLOUT;
	}

	epoll_ev.data.u64 = 0;
	epoll_ev.data.fd = fd;

	if (nmask != EG_EVENT_NONE) {
		epoll_ctl(data->epfd, EPOLL_CTL_MOD, fd, &epoll_ev);
	} else {
		epoll_ctl(data->epfd, EPOLL_CTL_DEL, fd, &epoll_ev);
	}
}

static int event_api_poll(EventLoop *loop, struct timeval *tvp)
{
	ApiData *data = (ApiData*)loop->api_data;
	struct epoll_event *epoll_ev;
	int retval, i, mask, events = 0;

	retval = epoll_wait(data->epfd, data->events, loop->size,
		tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);

	if (retval > 0)
	{
		events = retval;
		for (i = 0; i < events; i++)
		{
			mask = 0;
			epoll_ev = data->events + i;

			if (epoll_ev->events & EPOLLIN) {
				mask |= EG_EVENT_READABLE;
			}

			if (epoll_ev->events & EPOLLOUT) {
				mask |= EG_EVENT_WRITABLE;
			}

			if (epoll_ev->events & EPOLLERR) {
				mask |= EG_EVENT_WRITABLE;
			}

			if (epoll_ev->events & EPOLLHUP) {
				mask |= EG_EVENT_WRITABLE;
			}

			loop->fired[i].fd = epoll_ev->data.fd;
			loop->fired[i].mask = mask;
		}
	}

	return events;
}

static char *event_api_name(void)
{
	return "epoll";
}

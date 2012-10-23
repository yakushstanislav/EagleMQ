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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>

typedef struct ApiData {
	fd_set rfds, wfds;
	fd_set _rfds, _wfds;
} ApiData;

static int event_api_init(EventLoop *loop)
{
	ApiData *data;

	data = (ApiData*)xmalloc(sizeof(*data));

	FD_ZERO(&data->rfds);
	FD_ZERO(&data->wfds);

	loop->api_data = data;

	return 0;
}

static void event_api_free(EventLoop *loop)
{
	xfree(loop->api_data);
}

static int event_api_add(EventLoop *loop, int fd, int mask)
{
	ApiData *data;

	data = loop->api_data;

	if (mask & EG_EVENT_READABLE) {
		FD_SET(fd, &data->rfds);
	}

	if (mask & EG_EVENT_WRITABLE) {
		FD_SET(fd, &data->wfds);
	}

	return 0;
}

static void event_api_delete(EventLoop *loop, int fd, int mask)
{
	ApiData *data;

	data = loop->api_data;

	if (mask & EG_EVENT_READABLE) {
		FD_CLR(fd, &data->rfds);
	}

	if (mask & EG_EVENT_WRITABLE) {
		FD_CLR(fd, &data->wfds);
	}
}

static int event_api_poll(EventLoop *loop, struct timeval *tvp)
{
	ApiData *data;
	FileEvent *file_event;
	int retval, i, mask, events = 0;

	data = loop->api_data;

	memcpy(&data->_rfds, &data->rfds, sizeof(fd_set));
	memcpy(&data->_wfds, &data->wfds, sizeof(fd_set));

	retval = select(loop->maxfd + 1, &data->_rfds, &data->_wfds, NULL, tvp);

	if (retval > 0)
	{
		for (i = 0; i <= loop->maxfd; i++)
		{
			mask = 0;
			file_event = &loop->events[i];

			if (file_event->mask == EG_EVENT_NONE) {
				continue;
			}

			if (file_event->mask & EG_EVENT_READABLE && FD_ISSET(i, &data->_rfds)) {
				mask |= EG_EVENT_READABLE;
			}

			if (file_event->mask & EG_EVENT_WRITABLE && FD_ISSET(i, &data->_wfds)) {
				mask |= EG_EVENT_WRITABLE;
			}

			loop->fired[events].fd = i;
			loop->fired[events].mask = mask;
			events++;
		}
	}

	return events;
}

static char *event_api_name(void)
{
	return "select";
}

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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "xmalloc.h"
#include "utils.h"

#define xmalloc_update_stat_alloc(size)\
	if (xmalloc_state_lock)\
	{\
		pthread_mutex_lock(&memory_stat_mutex);\
		used_memory += size;\
		pthread_mutex_unlock(&memory_stat_mutex);\
	}\
	else\
	{\
		used_memory += size;\
	}

#define xmalloc_update_stat_free(size)\
	if (xmalloc_state_lock)\
	{\
		pthread_mutex_lock(&memory_stat_mutex);\
		used_memory -= size;\
		pthread_mutex_unlock(&memory_stat_mutex);\
	}\
	else\
	{\
		used_memory -= size;\
	}

static size_t used_memory = 0;
static int xmalloc_state_lock = 0;
pthread_mutex_t memory_stat_mutex = PTHREAD_MUTEX_INITIALIZER;

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		fatal("Error allocate memory");
	}

	xmalloc_update_stat_alloc(malloc_usable_size(ptr));

	return ptr;
}

void *xcalloc(size_t size)
{
	void *ptr = calloc(1, size);

	if (!ptr) {
		fatal("Error allocate memory");
	}

	xmalloc_update_stat_alloc(size);

	return ptr;
}

void xfree(void *ptr)
{
	if (!ptr) {
		return;
	}

	xmalloc_update_stat_free(malloc_usable_size(ptr));

	free(ptr);
}

void *xrealloc(void *ptr, size_t size)
{
	size_t old;
	void *newptr;

	if (!ptr) {
		return xmalloc(size);
	}

	old = malloc_usable_size(ptr);
	newptr = realloc(ptr, size);

	if (!newptr) {
		fatal("Error allocate memory");
	}

	xmalloc_update_stat_free(old);
	xmalloc_update_stat_alloc(malloc_usable_size(newptr));

	return newptr;
}

char *xstrdup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *ptr = xmalloc(len);

	memcpy(ptr, str, len);

	return ptr;
}

size_t xmalloc_used_memory(void)
{
	return used_memory;
}

void xmalloc_state_lock_on(void)
{
	xmalloc_state_lock = 1;
}

size_t xmalloc_memory_rss(void)
{
	int page = sysconf(_SC_PAGESIZE);
	size_t rss;
	char buf[4096];
	char filename[256];
	int fd, count;
	char *p, *x;

	snprintf(filename, 256, "/proc/%d/stat", getpid());
	if ((fd = open(filename, O_RDONLY)) == -1) {
		return 0;
	}

	if (read(fd, buf, 4096) <= 0) {
		close(fd);
		return 0;
	}

	close(fd);

	p = buf;
	count = 23;
	while (p && count--) {
		p = strchr(p, ' ');
		if (p) p++;
	}

	if (!p) {
		return 0;
	}

	x = strchr(p, ' ');
	if (!x) {
		return 0;
	}

	*x = '\0';

	rss = strtoll(p, NULL, 10);
	rss *= page;

	return rss;
}

float xmalloc_fragmentation_ratio(void)
{
	return (float)xmalloc_memory_rss()/xmalloc_used_memory();
}

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

#ifdef HAVE_MALLOC_SIZE
#define PREFIX_SIZE (0)
#else
#if defined(__sun) || defined(__sparc) || defined(__sparc__)
#define PREFIX_SIZE (sizeof(long long))
#else
#define PREFIX_SIZE (sizeof(size_t))
#endif
#endif

#if defined(_USE_TCMALLOC_)
#define malloc(size) tc_malloc(size)
#define calloc(count,size) tc_calloc(count,size)
#define realloc(ptr,size) tc_realloc(ptr,size)
#define free(ptr) tc_free(ptr)
#elif defined(_USE_JEMALLOC_)
#define malloc(size) je_malloc(size)
#define calloc(count,size) je_calloc(count,size)
#define realloc(ptr,size) je_realloc(ptr,size)
#define free(ptr) je_free(ptr)
#endif

#ifdef HAVE_ATOMIC
#define xmalloc_update_stat_add(__n) __sync_add_and_fetch(&used_memory, (__n))
#define xmalloc_update_stat_sub(__n) __sync_sub_and_fetch(&used_memory, (__n))
#else
#define xmalloc_update_stat_add(__n) do { \
	pthread_mutex_lock(&memory_stat_mutex); \
	used_memory += (__n); \
	pthread_mutex_unlock(&memory_stat_mutex); \
} while(0)

#define xmalloc_update_stat_sub(__n) do { \
	pthread_mutex_lock(&memory_stat_mutex); \
	used_memory -= (__n); \
	pthread_mutex_unlock(&memory_stat_mutex); \
} while(0)
#endif

#define xmalloc_update_stat_alloc(__n,__size) do { \
	size_t _n = (__n); \
	if (_n & (sizeof(long) - 1)) _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
	if (xmalloc_state_lock) { \
		xmalloc_update_stat_add(_n); \
	} else { \
		used_memory += _n; \
	} \
} while(0)

#define xmalloc_update_stat_free(__n) do { \
	size_t _n = (__n); \
	if (_n & (sizeof(long) - 1)) _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
	if (xmalloc_state_lock) { \
		xmalloc_update_stat_sub(_n); \
	} else { \
		used_memory -= _n; \
	} \
} while(0)

static size_t used_memory = 0;
static int xmalloc_state_lock = 0;
pthread_mutex_t memory_stat_mutex = PTHREAD_MUTEX_INITIALIZER;

void *xmalloc(size_t size)
{
	void *ptr = malloc(size + PREFIX_SIZE);

	if (!ptr) {
		fatal("Error allocate memory");
	}

#ifdef HAVE_MALLOC_SIZE
	xmalloc_update_stat_alloc(xmalloc_size(ptr), size);
	return ptr;
#else
	*((size_t*)ptr) = size;
	xmalloc_update_stat_alloc(size + PREFIX_SIZE, size);
	return (char*)ptr + PREFIX_SIZE;
#endif
}

void *xcalloc(size_t size)
{
	void *ptr = calloc(1, size + PREFIX_SIZE);

	if (!ptr) {
		fatal("Error allocate memory");
	}

#ifdef HAVE_MALLOC_SIZE
	xmalloc_update_stat_alloc(xmalloc_size(ptr), size);
	return ptr;
#else
	*((size_t*)ptr) = size;
	xmalloc_update_stat_alloc(size + PREFIX_SIZE, size);
	return (char*)ptr + PREFIX_SIZE;
#endif
}

void *xrealloc(void *ptr, size_t size)
{
#ifndef HAVE_MALLOC_SIZE
	void *realptr;
#endif
	size_t oldsize;
	void *newptr;

	if (!ptr) {
		return xmalloc(size);
	}

#ifdef HAVE_MALLOC_SIZE
	oldsize = xmalloc_size(ptr);
	newptr = realloc(ptr, size);

	if (!newptr) {
		fatal("Error allocate memory");
	}

	xmalloc_update_stat_free(oldsize);
	xmalloc_update_stat_alloc(xmalloc_size(newptr), size);

	return newptr;
#else
	realptr = (char*)ptr - PREFIX_SIZE;
	oldsize = *((size_t*)realptr);
	newptr = realloc(realptr, size + PREFIX_SIZE);

	if (!newptr) {
		fatal("Error allocate memory");
	}

	*((size_t*)newptr) = size;
	xmalloc_update_stat_free(oldsize);
	xmalloc_update_stat_alloc(size, size);

	return (char*)newptr + PREFIX_SIZE;
#endif
}

char *xstrdup(const char *str)
{
	size_t len = strlenz(str);
	char *ptr = xmalloc(len);

	memcpy(ptr, str, len);

	return ptr;
}

void xfree(void *ptr)
{
#ifndef HAVE_MALLOC_SIZE
	void *realptr;
	size_t oldsize;
#endif

	if (!ptr) {
		return;
	}

#ifdef HAVE_MALLOC_SIZE
	xmalloc_update_stat_free(xmalloc_size(ptr));
	free(ptr);
#else
	realptr = (char*)ptr - PREFIX_SIZE;
	oldsize = *((size_t*)realptr);
	xmalloc_update_stat_free(oldsize + PREFIX_SIZE);
	free(realptr);
#endif
}

#ifndef HAVE_MALLOC_SIZE
size_t xmalloc_size(void *ptr)
{
	void *realptr = (char*)ptr - PREFIX_SIZE;
	size_t size = *((size_t*)realptr);

	if (size & (sizeof(long) - 1)) {
		size += sizeof(long) - (size & (sizeof(long) - 1));
	}

	return size + PREFIX_SIZE;
}
#endif

size_t xmalloc_used_memory(void)
{
	size_t um;

	if (xmalloc_state_lock) {
#ifdef HAVE_ATOMIC
		um = __sync_add_and_fetch(&used_memory, 0);
#else
		pthread_mutex_lock(&memory_stat_mutex);
		um = used_memory;
		pthread_mutex_unlock(&memory_stat_mutex);
#endif
	} else {
		um = used_memory;
	}

	return um;
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

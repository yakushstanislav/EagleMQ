/*
   Copyright (c) 2009-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __XMALLOC_H__
#define __XMALLOC_H__

#include <stdlib.h>

#if defined(_USE_TCMALLOC_)
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define HAVE_MALLOC_SIZE 1
#define xmalloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif
#endif

#if defined(_USE_JEMALLOC_)
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define HAVE_MALLOC_SIZE 1
#define xmalloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif
#endif

void *xmalloc(size_t size);
void *xcalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str);
void xfree(void *ptr);
size_t xmalloc_used_memory(void);
void xmalloc_state_lock_on(void);
size_t xmalloc_memory_rss(void);
float xmalloc_fragmentation_ratio(void);

#ifndef HAVE_MALLOC_SIZE
size_t xmalloc_size(void *ptr);
#endif

#endif

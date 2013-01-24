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

#include <string.h>

#include "eagle.h"
#include "object.h"
#include "xmalloc.h"
#include "list.h"

Object *create_object(void *ptr, int size)
{
	Object *object = (Object*)xmalloc(sizeof(*object));

	object->data = ptr;
	object->size = size;
	object->refcount = 1;

	return object;
}

Object *create_dup_object(void *ptr, int size)
{
	Object *object = (Object*)xmalloc(sizeof(*object));

	object->data = xmalloc(size);
	object->size = size;
	object->refcount = 1;

	memcpy(object->data, ptr, size);

	return object;
}

void release_object(Object *object)
{
	xfree(object->data);
	xfree(object);
}

void increment_references_count(Object *object)
{
	object->refcount++;
}

void decrement_references_count(Object *object)
{
	if (object->refcount <= 1) {
		release_object(object);
	} else {
		object->refcount--;
	}
}

void free_object_list_handler(void *ptr)
{
	release_object(ptr);
}

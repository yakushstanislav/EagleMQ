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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "eagle.h"
#include "user.h"
#include "xmalloc.h"
#include "utils.h"
#include "list.h"

EagleUser *create_user(const char *name, const char *password, uint64_t perm)
{
	EagleUser *user = (EagleUser*)xmalloc(sizeof(*user));

	memcpy(user->name, name, 32);
	memcpy(user->password, password, 32);

	user->perm = perm;

	return user;
}

void delete_user(EagleUser *user)
{
	xfree(user);
}

EagleUser *find_user(List *list, const char *name, const char *password)
{
	EagleUser *user;
	ListNode *node;
	ListIterator iterator;

	list_rewind(list, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		user = EG_LIST_NODE_VALUE(node);

		if (password != NULL) {
			if (!strncmp(user->name, name, 32) && !strncmp(user->password, password, 32)) {
				return user;
			}
		} else {
			if (!strncmp(user->name, name, 32)) {
				return user;
			}
		}
	}

	return NULL;
}

void rename_user(EagleUser *user, const char *name)
{
	memcpy(user->name, name, 32);
}

void set_user_perm(EagleUser *user, uint64_t perm)
{
	user->perm = perm;
}

uint64_t get_user_perm(EagleUser *user)
{
	return user->perm;
}

void add_user_list(List *list, EagleUser *user)
{
	list_add_value_tail(list, user);
}

int delete_user_list(List *list, EagleUser *user)
{
	return list_delete_value(list, user);
}

void free_user_list_handler(void *ptr)
{
	delete_user(ptr);
}

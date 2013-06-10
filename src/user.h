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

#ifndef __USER_H__
#define __USER_H__

#include <stdint.h>

#include "list.h"

#define EG_USER_ALL_PERM 0x3F
#define EG_USER_SUPER_PERM 0x7F

#define EG_USER_QUEUE_PERM 0
#define EG_USER_ROUTE_PERM 1
#define EG_USER_CHANNEL_PERM 2
#define EG_USER_RESV3_PERM 3
#define EG_USER_RESV4_PERM 4
#define EG_USER_ADMIN_PERM 5
#define EG_USER_NOT_CHANGE_PERM 6

#define EG_USER_QUEUE_CREATE_PERM 20
#define EG_USER_QUEUE_DECLARE_PERM 21
#define EG_USER_QUEUE_EXIST_PERM 22
#define EG_USER_QUEUE_LIST_PERM 23
#define EG_USER_QUEUE_RENAME_PERM 24
#define EG_USER_QUEUE_SIZE_PERM 25
#define EG_USER_QUEUE_PUSH_PERM 26
#define EG_USER_QUEUE_GET_PERM 27
#define EG_USER_QUEUE_POP_PERM 28
#define EG_USER_QUEUE_CONFIRM_PERM 29
#define EG_USER_QUEUE_SUBSCRIBE_PERM 30
#define EG_USER_QUEUE_UNSUBSCRIBE_PERM 31
#define EG_USER_QUEUE_PURGE_PERM 32
#define EG_USER_QUEUE_DELETE_PERM 33

#define EG_USER_ROUTE_CREATE_PERM 34
#define EG_USER_ROUTE_EXIST_PERM 35
#define EG_USER_ROUTE_LIST_PERM 36
#define EG_USER_ROUTE_KEYS_PERM 37
#define EG_USER_ROUTE_RENAME_PERM 38
#define EG_USER_ROUTE_BIND_PERM 39
#define EG_USER_ROUTE_UNBIND_PERM 40
#define EG_USER_ROUTE_PUSH_PERM 41
#define EG_USER_ROUTE_DELETE_PERM 42

#define EG_USER_CHANNEL_CREATE_PERM 43
#define EG_USER_CHANNEL_EXIST_PERM 44
#define EG_USER_CHANNEL_LIST_PERM 45
#define EG_USER_CHANNEL_RENAME_PERM 46
#define EG_USER_CHANNEL_PUBLISH_PERM 47
#define EG_USER_CHANNEL_SUBSCRIBE_PERM 48
#define EG_USER_CHANNEL_PSUBSCRIBE_PERM 49
#define EG_USER_CHANNEL_UNSUBSCRIBE_PERM 50
#define EG_USER_CHANNEL_PUNSUBSCRIBE_PERM 51
#define EG_USER_CHANNEL_DELETE_PERM 52

typedef struct EagleUser {
	char name[32];
	char password[32];
	uint64_t perm;
} EagleUser;

EagleUser *create_user(const char *name, const char *password, uint64_t perm);
void delete_user(EagleUser *user);
EagleUser *find_user(List *list, const char *name, const char *password);
void rename_user(EagleUser *user, const char *name);
void set_user_perm(EagleUser *user, uint64_t perm);
uint64_t get_user_perm(EagleUser *user);
void free_user_list_handler(void *ptr);

#endif

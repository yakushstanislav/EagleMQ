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

#ifndef __EAGLE_LIB_H__
#define __EAGLE_LIB_H__

#include <stdint.h>

#include "event.h"
#include "network.h"
#include "list.h"
#include "user.h"

#define EG_NOTUSED(X) ((void)X)

#define EG_STATUS_OK 0
#define EG_STATUS_ERR -1

#define EG_DEFAULT_ADDR	"127.0.0.1"
#define EG_DEFAULT_PORT 7851
#define EG_DEFAULT_UNIX_SOCKET NULL
#define EG_DEFAULT_ADMIN_NAME "eagle"
#define EG_DEFAULT_ADMIN_PASSWORD "eagle"
#define EG_DEFAULT_MAX_CLIENTS 16384
#define EG_DEFAULT_TIMEOUT 0
#define EG_DEFAULT_DAEMONIZE 0
#define EG_DEFAULT_PID_PATH NULL
#define EG_DEFAULT_LOG_PATH "eaglemq.log"

#define EG_BUF_SIZE 32768

#define EG_MAX_BUF_SIZE 2147483647
#define EG_MAX_MSG_COUNT 4294967295
#define EG_MAX_MSG_SIZE 2147483647

#define BIT_SET(a, b) ((a) |= (1<<(b)))
#define BIT_CHECK(a, b) ((a) & (1<<(b)))

typedef struct EagleClient
{
	int fd;
	uint64_t perm;
	char *request;
	size_t length;
	size_t pos;
	int offset;
	char *buffer;
	size_t bodylen;
	size_t nread;
	List *responses;
	List *declared_queues;
	List *subscribed_queues;
	size_t sentlen;
	time_t last_action;
} EagleClient;

typedef struct EagleServer
{
	EventLoop *loop;
	int fd;
	int sfd;
	long long ufd;
	char *addr;
	int port;
	char *unix_socket;
	mode_t unix_perm;
	size_t max_clients;
	int timeout;
	char error[NET_ERR_LEN];
	List *clients;
	List *users;
	List *queues;
	time_t now_time;
	time_t start_time;
	int daemonize;
	char *pidfile;
	char *logfile;
	int shutdown;
	unsigned int rx;
	unsigned int tx;
} EagleServer;

extern EagleServer *server;

#endif

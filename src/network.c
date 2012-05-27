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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "network.h"

static void net_set_error(char *err, const char *fmt,...)
{
	va_list list;

	if (!err) {
		return;
	}

	va_start(list, fmt);
	vsnprintf(err, NET_ERR_LEN, fmt, list);
	va_end(list);
}

static int net_listen(char *err, int sock, struct sockaddr *sa, socklen_t len)
{
	if (bind(sock, sa, len) == -1) {
		net_set_error(err, "bind: %s", strerror(errno));
		warning("bind socket error:\n%s", err);
		close(sock);
		return EG_NET_ERR;
	}

	if (listen(sock, 511) == -1) { /* the magic 511 constant is from nginx and redis */
		net_set_error(err, "listen: %s", strerror(errno));
		close(sock);
		return EG_NET_ERR;
	}

	return EG_NET_OK;
}

int net_set_nonblock(char *err, int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		net_set_error(err, "fcntl: %s", strerror(errno));
		return EG_NET_OK;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		net_set_error(err, "fcntl: %s", strerror(errno));
        return EG_NET_ERR;
	}

	return EG_NET_ERR;
}

int net_tcp_nodelay(char *err, int fd)
{
	int yes = 1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
		net_set_error(err, "setsockopt: %s", strerror(errno));
		return EG_NET_ERR;
	}

	return EG_NET_OK;
}

static int net_create_socket(char *err, int domain)
{
	int sock, on = 1;

	if ((sock = socket(domain, SOCK_STREAM, 0)) == -1) {
		net_set_error(err, "socket: %s", strerror(errno));
		return EG_NET_ERR;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		net_set_error(err, "setsockopt: %s", strerror(errno));
		return EG_NET_ERR;
	}

	return sock;
}

int net_tcp_server(char *err, const char *addr, int port)
{
	int sock;
	struct sockaddr_in sa;

	if ((sock = net_create_socket(err, AF_INET)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	memset(&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (addr && inet_aton(addr, &sa.sin_addr) == 0) {
		net_set_error(err, "Invalid bind address");
		close(sock);
		return EG_NET_ERR;
	}

	if (net_listen(err, sock, (struct sockaddr*)&sa, sizeof(sa)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	return sock;
}

int net_unix_server(char *err, const char *path, mode_t perm)
{
	int sock;
	struct sockaddr_un sa;

	if ((sock = net_create_socket(err, AF_LOCAL)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_LOCAL;

	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

	if (net_listen(err, sock, (struct sockaddr*)&sa, sizeof(sa)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	if (perm) {
		chmod(sa.sun_path, perm);
	}

	return sock;
}

static int net_generic_accept(char *err, int sock, struct sockaddr *sa, socklen_t *len)
{
	int fd;

	while (1)
	{
		fd = accept(sock, sa, len);
		if (fd == -1)
		{
			if (errno == EINTR) {
				continue;
			} else {
				net_set_error(err, "accept: %s", strerror(errno));
				return EG_NET_ERR;
			}
		}
		break;
	}

	return fd;
}

int net_tcp_accept(char *err, int sock, char *ip, int *port)
{
	int fd;
	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);

	if ((fd = net_generic_accept(err, sock, (struct sockaddr*)&sa, &sa_len)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	if (ip) {
		strcpy(ip, inet_ntoa(sa.sin_addr));
	}

	if (port) {
		*port = ntohs(sa.sin_port);
	}

	return fd;
}

int net_unix_accept(char *err, int sock)
{
	int fd;
	struct sockaddr_un sa;
	socklen_t salen = sizeof(sa);

	if ((fd = net_generic_accept(err, sock, (struct sockaddr*)&sa, &salen)) == EG_NET_ERR) {
		return EG_NET_ERR;
	}

	return fd;
}

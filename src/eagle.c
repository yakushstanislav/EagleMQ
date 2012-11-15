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

#include "fmacros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <limits.h>
#include <float.h>

#include "eagle.h"
#include "logo.h"
#include "version.h"
#include "xmalloc.h"
#include "handlers.h"
#include "utils.h"
#include "event.h"
#include "network.h"
#include "list.h"
#include "user.h"
#include "queue_t.h"

EagleServer *server;

char *name, *password;

void create_pid_file(const char *pidfile)
{
	FILE *fp = fopen(pidfile, "w");

	if (fp) {
		fprintf(fp, "%d\n", (int)getpid());
		fclose(fp);
	}
}

void daemonize(void)
{
	int fd;

	if (fork() != 0) {
		exit(0);
	}

	setsid();

	if ((fd = open("/dev/null", O_RDWR, 0)) != -1)
	{
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO) {
			close(fd);
		}
	}
}

void unblock_open_files_limit(void)
{
	rlim_t maxfiles = server->max_clients + 32;
	rlim_t oldlimit;
	struct rlimit limit;

	if (maxfiles < 1024) {
		maxfiles = 1024;
	}

	if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
		warning("Error getrlimit", strerror(errno));
		server->max_clients = 1024 - 32;
	} else {
		oldlimit = limit.rlim_cur;

		if (oldlimit < maxfiles) {
			limit.rlim_cur = maxfiles;
			limit.rlim_max = maxfiles;

			if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
				server->max_clients = oldlimit - 32;
			}
		}
    }
}

int server_updater(EventLoop *loop, long long id, void *data)
{
	EG_NOTUSED(loop);
	EG_NOTUSED(id);
	EG_NOTUSED(data);

	server->now_time = time(NULL);

	client_timeout();

	if (server->shutdown) {
		stop_event_loop(server->loop);
	}

	return 100;
}

void sigterm_handler(int sig)
{
	EG_NOTUSED(sig);

	warning("Received SIGTERM...");

	server->shutdown = 1;
}

void setup_signals(void)
{
	struct sigaction sig;

	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	sigemptyset(&sig.sa_mask);
	sig.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
	sig.sa_handler = sigterm_handler;

	sigaction(SIGTERM, &sig, NULL);
}

void init_admin(void)
{
	EagleUser *admin;

	admin = create_user(name, password, EG_USER_SUPER_PERM);
	add_user_list(server->users, admin);
}

void init_server(void)
{
	if (server->daemonize) {
		daemonize();
	}

	if (server->pidfile) {
		create_pid_file(server->pidfile);
	}

	if (server->logfile) {
		enable_log(server->logfile);
	}

	unblock_open_files_limit();

	server->loop = create_event_loop(server->max_clients + 1);

	if (server->port != 0) {
		if ((server->fd = net_tcp_server(server->error, server->addr, server->port)) == EG_NET_ERR) {
			fatal("Error create server: %s\n%s", server->addr, server->error);
		}
	}

	if (server->unix_socket != NULL) {
		unlink(server->unix_socket);
		if ((server->sfd = net_unix_server(server->error, server->unix_socket, server->unix_perm)) == EG_NET_ERR) {
			fatal("Opening socket: %s\n", server->error);
		}
	}

	if (server->fd < 0 && server->sfd < 0) {
		fatal("Error configure server");
	}

	if (server->fd > 0 && create_file_event(server->loop, server->fd, EG_EVENT_READABLE, accept_tcp_handler, NULL) == EG_EVENT_ERR) {
		fatal("Error create file event");
	}

	if (server->sfd > 0 && create_file_event(server->loop, server->sfd, EG_EVENT_READABLE, accept_unix_handler, NULL) == EG_EVENT_ERR) {
		fatal("Error create file event");
	}

	server->ufd = create_time_event(server->loop, 1, server_updater, NULL, NULL);
}

void destroy_server()
{
	if (server->pidfile) {
		unlink(server->pidfile);
	}

	if (server->unix_socket) {
		unlink(server->unix_socket);
	}

	list_release(server->clients);
	list_release(server->users);
	list_release(server->queues);

	xfree(server);

	disable_log();
}

void init_server_config(void)
{
	server = (EagleServer*)xmalloc(sizeof(*server));

	name = EG_DEFAULT_ADMIN_NAME;
	password = EG_DEFAULT_ADMIN_PASSWORD;

	server->fd = 0;
	server->sfd = 0;
	server->ufd = 0;
	server->addr = EG_DEFAULT_ADDR;
	server->port = EG_DEFAULT_PORT;
	server->unix_socket = EG_DEFAULT_UNIX_SOCKET;
	server->max_clients = EG_DEFAULT_MAX_CLIENTS;
	server->timeout = EG_DEFAULT_TIMEOUT;
	server->clients = list_create();
	server->users = list_create();
	server->queues = list_create();
	server->now_time = time(NULL);
	server->start_time = time(NULL);
	server->daemonize = EG_DEFAULT_DAEMONIZE;
	server->pidfile = EG_DEFAULT_PID_PATH;
	server->logfile = EG_DEFAULT_LOG_PATH;
	server->shutdown = 0;

	EG_LIST_SET_FREE_METHOD(server->users, free_user_list_handler);
	EG_LIST_SET_FREE_METHOD(server->queues, free_queue_list_handler);
}

void show_logo(void)
{
	printf(ascii_logo,
		EAGLE_VERSION,
		server->addr,
		server->port,
		get_event_api_name());
}

void usage(void)
{
	printf(
		"EagleMQ %s\n"
		"Usage: eaglemq [options]\n"
		"-h <hostname> - server hostname(default: %s)\n"
		"-p <port> - server port(default: %d)\n"
		"-u <unix socket> - create unix socket\n"
		"-d <daemon> - run server as daemon\n"
		"-l <log> - path to log file(default: %s)\n"
		"-m <max> - maximum connections on the server(default: %d)\n"
		"-t <timeout> - timeout for not active connections(default: %d sec)\n"
		"-pid <path to the PID file> - path to the PID file of server\n"
		"-name <name> - administrator name(default: %s)\n"
		"-password <password> - administrator password(default: %s)\n"
		"--v or --version - output version and exit\n"
		"-h or --help - output this help and exit\n",
			EAGLE_VERSION, EG_DEFAULT_ADDR, EG_DEFAULT_PORT, EG_DEFAULT_LOG_PATH,
			EG_DEFAULT_MAX_CLIENTS, EG_DEFAULT_TIMEOUT,
			EG_DEFAULT_ADMIN_NAME, EG_DEFAULT_ADMIN_PASSWORD);
}

void parse_args(int argc, char *argv[])
{
	int i, last_arg;

	for (i = 1; i < argc; i++)
	{
		last_arg = i == argc - 1;

		if (!strcmp(argv[i], "-h") && !last_arg) {
			server->addr = argv[i + 1];
		} else if (!strcmp(argv[i], "-p") && !last_arg) {
			server->port = atoi(argv[i + 1]);
		} else if (!strcmp(argv[i], "-u") && !last_arg) {
			server->unix_socket = argv[i + 1];
		} else if (!strcmp(argv[i], "-d")) {
			server->daemonize = 1;
		} else if (!strcmp(argv[i], "-l") && !last_arg) {
			server->logfile = argv[i + 1];
		} else if (!strcmp(argv[i], "-m") && !last_arg) {
			server->max_clients = atoi(argv[i + 1]);
		} else if (!strcmp(argv[i], "-t") && !last_arg) {
			server->timeout = atoi(argv[i + 1]);
		} else if (!strcmp(argv[i], "-pid") && !last_arg) {
			server->pidfile = argv[i + 1];
		} else if (!strcmp(argv[i], "-name") && !last_arg) {
			if (strlen(argv[i + 1]) >= 32) fatal("Error name length");
			name = argv[i + 1];
		} else if (!strcmp(argv[i], "-password") && !last_arg) {
			if (strlen(argv[i + 1]) >= 32) fatal("Error password length");
			password = argv[i + 1];
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			info("EagleMQ %s", EAGLE_VERSION);
			exit(0);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
			exit(0);
		}
	}
}

int main(int argc, char *argv[])
{
	setup_signals();
	xmalloc_state_lock_on();

	init_server_config();
	parse_args(argc, argv);

	show_logo();

	init_server();
	init_admin();

	start_main_loop(server->loop);
	delete_event_loop(server->loop);

	destroy_server();

	return 0;
}

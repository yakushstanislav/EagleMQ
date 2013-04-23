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
#include <sys/resource.h>
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
#include "route_t.h"
#include "storage.h"
#include "config.h"

EagleServer *server;

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

int linux_overcommit_memory_value(void)
{
	FILE *fp;
	char buf[64];

	if ((fp = fopen("/proc/sys/vm/overcommit_memory", "r")) == NULL)
		return -1;

	if (fgets(buf, 64, fp) == NULL)
	{
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return atoi(buf);
}

void linux_overcommit_memory_warning(void)
{
	if (linux_overcommit_memory_value() == 0) {
		warning("Please, for correct work set the overcommit memory value to 1. "
			"Run the command 'sysctl vm.overcommit_memory=1'.");
	}
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

long long mstime(void)
{
	struct timeval tv;
	long long mst;

	gettimeofday(&tv, NULL);

	mst = tv.tv_sec * 1000;
	mst += tv.tv_usec / 1000;

	return mst;
}

void check_memory(void)
{
	if (server->max_memory)
	{
		if (xmalloc_used_memory() > server->max_memory)
		{
			if ((server->now_time - server->last_memcheck) > EG_MEMORY_CHECK_TIMEOUT)
			{
				warning("Used memory: %u, limit: %u",
					xmalloc_used_memory(), server->max_memory);

				server->last_memcheck = server->now_time;
			}

			server->nomemory = 1;
		}
		else
		{
			server->nomemory = 0;
		}
	}
}

void process_queues_messages(void)
{
	ListNode *node;
	ListIterator iterator;
	Queue_t *queue_t;

	list_rewind(server->queues, &iterator);
	while ((node = list_next_node(&iterator)) != NULL)
	{
		queue_t = EG_LIST_NODE_VALUE(node);
		process_expired_messages_queue_t(queue_t, server->now_timems);
		process_unconfirmed_messages_queue_t(queue_t, server->now_timems);
	}
}

void storage_timeout(void)
{
	pid_t pid;
	int stat;

	if (server->child_pid != -1)
	{
		if ((pid = wait3(&stat, WNOHANG, NULL)) != 0)
		{
			if (WEXITSTATUS(stat) != EG_STATUS_OK) {
				warning("Error background saving data in %s", server->storage);
			}

			server->child_pid = -1;
		}
	}

	if (server->storage_timeout && server->child_pid == -1)
	{
		if ((server->now_time - server->last_save) > server->storage_timeout)
		{
			wlog("Save all data to the storage...");
			if (storage_save_background(server->storage) != EG_STATUS_OK) {
				warning("Error saving data in %s", server->storage);
			}

			server->last_save = server->now_time;
		}
	}
}

int server_updater(EventLoop *loop, long long id, void *data)
{
	EG_NOTUSED(loop);
	EG_NOTUSED(id);
	EG_NOTUSED(data);

	server->now_time = time(NULL);
	server->now_timems = mstime();
	server->msg_counter = 0;

	check_memory();
	client_timeout();
	process_queues_messages();
	storage_timeout();

	if (server->shutdown) {
		stop_event_loop(server->loop);
	}

	return 100;
}

void init_admin(void)
{
	EagleUser *admin;

	admin = create_user(server->name, server->password, EG_USER_SUPER_PERM);
	list_add_value_tail(server->users, admin);
}

void init_storage()
{
	if (access(server->storage, F_OK) == -1)
	{
		if (storage_save(server->storage) != EG_STATUS_OK) {
			warning("Error init storage %s\n", server->storage);
		}
	}

	if (storage_load(server->storage) != EG_STATUS_OK) {
		warning("Error loading data from %s", server->storage);
	}
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

	if (server->port != 0)
	{
		server->fd = net_tcp_server(server->error, server->addr, server->port);
		if (server->fd == EG_NET_ERR) {
			fatal("Error create server: %s\n%s", server->addr, server->error);
		}
	}

	if (server->unix_socket != NULL)
	{
		unlink(server->unix_socket);

		server->sfd = net_unix_server(server->error, server->unix_socket, server->unix_perm);
		if (server->sfd == EG_NET_ERR) {
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
	if (server->child_pid != -1)
	{
		kill(server->child_pid, SIGKILL);
		remove_temp_file(server->child_pid);
	}

	if (server->fd > 0) {
		close(server->fd);
	}

	if (server->sfd > 0) {
		close(server->sfd);
	}

	if (server->pidfile) {
		unlink(server->pidfile);
	}

	if (server->unix_socket) {
		unlink(server->unix_socket);
	}

	disable_log();
}

void init_config(void)
{
	if (access(server->config, F_OK) != -1)
	{
		if (config_load(server->config) != EG_STATUS_OK)
		{
			fatal("Error load config file %s", server->config);
		}
	}
	else
	{
		warning("Config file %s don't exist", server->config);
	}
}

void init_server_config(void)
{
	server = (EagleServer*)xmalloc(sizeof(*server));

	server->fd = 0;
	server->sfd = 0;
	server->ufd = 0;
	server->addr = xstrdup(EG_DEFAULT_ADDR);
	server->port = EG_DEFAULT_PORT;
	server->unix_socket = NULL;
	server->unix_perm = 0;
	server->child_pid = -1;
	server->name = xstrdup(EG_DEFAULT_ADMIN_NAME);
	server->password = xstrdup(EG_DEFAULT_ADMIN_PASSWORD);
	server->max_clients = EG_DEFAULT_MAX_CLIENTS;
	server->max_memory = EG_DEFAULT_MAX_MEMORY;
	server->client_timeout = EG_DEFAULT_CLIENT_TIMEOUT;
	server->storage_timeout = EG_DEFAULT_SAVE_TIMEOUT;
	server->clients = list_create();
	server->users = list_create();
	server->queues = list_create();
	server->routes = list_create();
	server->now_time = time(NULL);
	server->now_timems = mstime();
	server->start_time = time(NULL);
	server->last_save = time(NULL);
	server->last_memcheck = time(NULL);
	server->nomemory = 0;
	server->msg_counter = 0;
	server->daemonize = EG_DEFAULT_DAEMONIZE;
	server->storage = xstrdup(EG_DEFAULT_STORAGE_PATH);
	server->pidfile = NULL;
	server->logfile = xstrdup(EG_DEFAULT_LOG_PATH);
	server->config = EG_DEFAULT_CONFIG_PATH;
	server->shutdown = 0;

	EG_LIST_SET_FREE_METHOD(server->users, free_user_list_handler);
	EG_LIST_SET_FREE_METHOD(server->queues, free_queue_list_handler);
	EG_LIST_SET_FREE_METHOD(server->routes, free_route_list_handler);
}

void destroy_server_config(void)
{
	xfree(server->addr);
	xfree(server->name);
	xfree(server->password);
	xfree(server->storage);
	xfree(server->logfile);

	if (server->unix_socket)
		xfree(server->unix_socket);

	if (server->pidfile)
		xfree(server->pidfile);

	list_release(server->clients);
	list_release(server->users);
	list_release(server->queues);
	list_release(server->routes);

	xfree(server);
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
		"Usage: eaglemq [path to config file] [key:value,...]\n"
		"--addr - the server address (default: %s)\n"
		"--port - the server port (default: %d)\n"
		"--unix-socket - the path to unix socket\n"
		"--admin-name - the administrator name (default: %s)\n"
		"--admin-password - the administrator password (default: %s)\n"
		"--daemonize - run the server as daemon [on|off]\n"
		"--pid-file - path to the PID file\n"
		"--log-file - path to the log file (default: %s)\n"
		"--storage-file - path to the storage file (default: %s)\n"
		"--max-clients - maximum connections on the server (default: %d)\n"
		"--max-memory - max memory usage limit (default: %d)\n"
		"--save-timeout - timeout for save data to the storage (default: %d sec)\n"
		"--client-timeout - timeout to kill not active clients (default: %d sec)\n",
			EAGLE_VERSION, EG_DEFAULT_ADDR, EG_DEFAULT_PORT,
			EG_DEFAULT_ADMIN_NAME, EG_DEFAULT_ADMIN_PASSWORD,
			EG_DEFAULT_LOG_PATH, EG_DEFAULT_STORAGE_PATH,
			EG_DEFAULT_MAX_CLIENTS, EG_DEFAULT_MAX_MEMORY,
			EG_DEFAULT_SAVE_TIMEOUT, EG_DEFAULT_CLIENT_TIMEOUT);
}

void parse_args(int argc, char *argv[])
{
	int position = 1;

	if (argc >= 2)
	{
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			usage();
			exit(0);
		} else {
			server->config = argv[1];
			position = 2;
			init_config();
		}
	}

	for (; position < argc; position += 2)
	{
		if (position == argc - 1)
			fatal("Error parse command line. Key %s must have value", argv[position]);

		if (strncmp(argv[position], "--", 2))
			fatal("Error parse command line. Use prefix \'--\' for key");

		if (config_parse_key_value(argv[position] + 2, argv[position + 1]) != EG_STATUS_OK)
			fatal("Error parse command line");
	}
}

int main(int argc, char *argv[])
{
	setup_signals();
	xmalloc_state_lock_on();

	init_server_config();

	parse_args(argc, argv);

	show_logo();

	linux_overcommit_memory_warning();

	init_server();
	init_admin();
	init_storage();

	wlog("Server started (version: %s)", EAGLE_VERSION);

	start_main_loop(server->loop);

	delete_time_event(server->loop, server->ufd);
	delete_event_loop(server->loop);

	destroy_server();
	destroy_server_config();

	return 0;
}

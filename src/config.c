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

#include "eagle.h"
#include "config.h"
#include "xmalloc.h"
#include "utils.h"

static void set_value_string(char **dest, char *value)
{
	if (*dest)
		xfree(*dest);

	*dest = xstrdup(value);
}

static int parse_on_off(char *value)
{
	int result = 0;

	if (!strcmp(value, "on"))
		result = 1;

	return result;
}

int config_parse_key_value(char *key, char *value)
{
	int err;
	long long max_memory;

	if (!strcmp(key, "addr")) {
		set_value_string(&server->addr, value);
	} else if (!strcmp(key, "port")) {
		server->port = atoi(value);
	} else if (!strcmp(key, "unix-socket")) {
		set_value_string(&server->unix_socket, value);
	} else if (!strcmp(key, "admin-name")) {
		set_value_string(&server->name, value);
	} else if (!strcmp(key, "admin-password")) {
		set_value_string(&server->password, value);
	} else if (!strcmp(key, "daemonize")) {
		server->daemonize = parse_on_off(value);
	} else if (!strcmp(key, "pid-file")) {
		set_value_string(&server->pidfile, value);
	} else if (!strcmp(key, "log-file")) {
		set_value_string(&server->logfile, value);
	} else if (!strcmp(key, "storage-file")) {
		set_value_string(&server->storage, value);
	} else if (!strcmp(key, "max-clients")) {
		server->max_clients = atoi(value);
	} else if (!strcmp(key, "max-memory")) {
		max_memory = memtoll(value, &err);
		if (err) return EG_STATUS_ERR;
		server->max_memory = max_memory;
	} else if (!strcmp(key, "save-timeout")) {
		server->storage_timeout = atoi(value);
	} else if (!strcmp(key, "client-timeout")) {
		server->client_timeout = atoi(value);
	} else {
		info("Error parse key: %s", key);
		return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;
}

static int parse_config_line(char *line)
{
	char *ptr;
	char *key = NULL;
	char *value = NULL;

	if (line[0] == '#' || line[0] == '\0' || line[0] == '\n')
		return EG_STATUS_OK;

	ptr = strtok(line, "\t\n ");
	while (ptr != NULL)
	{
		if (!key) {
			key = ptr;
		} else if (!value) {
			value = ptr;
		} else {
			return EG_STATUS_ERR;
		}

		ptr = strtok(NULL, "\t\n ");
	}

	if (key && value)
	{
		if (config_parse_key_value(key, value) != EG_STATUS_OK)
			return EG_STATUS_ERR;
	} else {
		return EG_STATUS_ERR;
	}

	return EG_STATUS_OK;
}

int config_load(const char *filename)
{
	char line[512];
	FILE *fp;

	fp = fopen(filename, "r");
	if (!fp)
		return EG_STATUS_ERR;

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		if (parse_config_line(line) != EG_STATUS_OK)
		{
			fclose(fp);
			return EG_STATUS_ERR;
		}
	}

	fclose(fp);

	return EG_STATUS_OK; 
}

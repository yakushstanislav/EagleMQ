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
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "eagle.h"
#include "utils.h"

int log_init = 0;
FILE *fd;

void enable_log(const char *logfile)
{
	fd = fopen(logfile, "a");

	if (fd == NULL) {
		warning("Error create log file");
		return;
	}

	log_init = 1;
}

void disable_log()
{
	if (log_init) {
		log_init = 0;
		fclose(fd);
	}
}

static void output_message(int level, const char *fmt, va_list args)
{
	time_t now_time;
	char msg[MESSAGE_BUFFER_SIZE];
	char log[MESSAGE_BUFFER_SIZE];

	vsnprintf(msg, sizeof(msg), fmt, args);

	if (level != LOG_ONLY_LEVEL)
	{
		if (level == FATAL_LEVEL || level == WARNING_LEVEL) {
			fprintf(stderr, "%s\n", msg);
		} else {
			fprintf(stdout, "%s\n", msg);
		}
	}

	if (log_init) {
		time(&now_time);
		sprintf(log, "%s%s\n", ctime(&now_time), msg);
		fputs(log, fd);
		fflush(fd);
	}
}

void warning(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	output_message(WARNING_LEVEL, fmt, args);
	va_end(args);
}

void info(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	output_message(INFO_LEVEL, fmt, args);
	va_end(args);
}

void fatal(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	output_message(FATAL_LEVEL, fmt, args);
	va_end(args);

	exit(EG_STATUS_ERR);
}

void wlog(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	output_message(LOG_ONLY_LEVEL, fmt, args);
	va_end(args);
}

int check_input_buffer1(char *buffer, size_t size)
{
	char *ptr = buffer, *end_ptr = buffer + size;
	int length = 0;

	while (ptr < end_ptr) {
		if (*++ptr == '\0') {
			length = ptr - buffer;
			break;
		}
	}

	if (!length) {
		return 0;
	}

	for (ptr = buffer; ptr < (buffer + length); ptr++)
	{
		if (!IS_ALPHANUM(*ptr) && !IS_EXTRA(*ptr)) {
			return 0;
		}
	}

	return length;
}

int check_input_buffer2(char *buffer, size_t size)
{
	char *ptr = buffer, *end_ptr = buffer + size;
	int length = 0;
	int extra = 0;

	while (ptr < end_ptr) {
		if (*++ptr == '\0') {
			length = ptr - buffer;
			break;
		}
	}

	if (!length) {
		return 0;
	}

	if (!IS_ALPHA(*buffer) && *buffer != '_' && *buffer != '.') {
		return 0;
	}

	if (*buffer == '_' || *buffer == '.') {
		extra = 1;
	}

	for (ptr = buffer + 1; ptr < (buffer + length - 1); ptr++)
	{
		if (IS_EXTRA(*ptr) && extra) {
			return 0;
		}

		if (!IS_ALPHANUM(*ptr) && !IS_EXTRA(*ptr)) {
			return 0;
		}

		extra = 0;
		if (IS_EXTRA(*ptr)) {
			extra = 1;
		}
	}

	return length;
}

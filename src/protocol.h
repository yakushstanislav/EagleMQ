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

#ifndef __PROTOCOL_BINARY_H__
#define __PROTOCOL_BINARY_H__

#include <stdint.h>

#define EG_FLUSH_USER_FLAG 0
#define EG_FLUSH_QUEUE_FLAG 1

typedef enum ProtocolBinaryMagic {
	EG_PROTOCOL_REQ = 0x70,
	EG_PROTOCOL_RES = 0x80,
	EG_PROTOCOL_EVENT = 0x90
} ProtocolBinaryMagic;

typedef enum ProtocolCommand {
	/* system commands */
	EG_PROTOCOL_CMD_AUTH = 0x1,
	EG_PROTOCOL_CMD_PING = 0x2,
	EG_PROTOCOL_CMD_STAT = 0x3,
	EG_PROTOCOL_CMD_FLUSH = 0x4,
	EG_PROTOCOL_CMD_DISCONNECT = 0x5,

	/* user control commands */
	EG_PROTOCOL_CMD_USER_CREATE = 0x10,
	EG_PROTOCOL_CMD_USER_LIST = 0x11,
	EG_PROTOCOL_CMD_USER_RENAME = 0x12,
	EG_PROTOCOL_CMD_USER_SET_PERM = 0x13,
	EG_PROTOCOL_CMD_USER_DELETE = 0x14,

	/* queue commands */
	EG_PROTOCOL_CMD_QUEUE_CREATE = 0x20,
	EG_PROTOCOL_CMD_QUEUE_DECLARE = 0x21,
	EG_PROTOCOL_CMD_QUEUE_EXIST = 0x22,
	EG_PROTOCOL_CMD_QUEUE_LIST = 0x23,
	EG_PROTOCOL_CMD_QUEUE_SIZE = 0x24,
	EG_PROTOCOL_CMD_QUEUE_PUSH = 0x25,
	EG_PROTOCOL_CMD_QUEUE_GET = 0x26,
	EG_PROTOCOL_CMD_QUEUE_POP = 0x27,
	EG_PROTOCOL_CMD_QUEUE_SUBSCRIBE = 0x28,
	EG_PROTOCOL_CMD_QUEUE_UNSUBSCRIBE = 0x29,
	EG_PROTOCOL_CMD_QUEUE_PURGE = 0x30,
	EG_PROTOCOL_CMD_QUEUE_DELETE = 0x31
} ProtocolCommand;

typedef enum ProtocolResponseStatus {
	EG_PROTOCOL_SUCCESS = 0x1,
	EG_PROTOCOL_SUCCESS_AUTH = 0x2,
	EG_PROTOCOL_SUCCESS_PING = 0x3,
	EG_PROTOCOL_SUCCESS_STAT = 0x4,
	EG_PROTOCOL_SUCCESS_FLUSH = 0x5,
	EG_PROTOCOL_SUCCESS_USER_CREATE = 0x10,
	EG_PROTOCOL_SUCCESS_USER_LIST = 0x11,
	EG_PROTOCOL_SUCCESS_USER_RENAME = 0x12,
	EG_PROTOCOL_SUCCESS_USER_SET_PERM = 0x13,
	EG_PROTOCOL_SUCCESS_USER_DELETE = 0x14,
	EG_PROTOCOL_SUCCESS_QUEUE_CREATE = 0x20,
	EG_PROTOCOL_SUCCESS_QUEUE_DECLARE = 0x21,
	EG_PROTOCOL_SUCCESS_QUEUE_EXIST = 0x22,
	EG_PROTOCOL_SUCCESS_QUEUE_LIST = 0x23,
	EG_PROTOCOL_SUCCESS_QUEUE_SIZE = 0x24,
	EG_PROTOCOL_SUCCESS_QUEUE_PUSH = 0x25,
	EG_PROTOCOL_SUCCESS_QUEUE_GET = 0x26,
	EG_PROTOCOL_SUCCESS_QUEUE_POP = 0x27,
	EG_PROTOCOL_SUCCESS_QUEUE_SUBSCRIBE = 0x28,
	EG_PROTOCOL_SUCCESS_QUEUE_UNSUBSCRIBE = 0x29,
	EG_PROTOCOL_SUCCESS_QUEUE_PURGE = 0x30,
	EG_PROTOCOL_SUCCESS_QUEUE_DELETE = 0x31,
	EG_PROTOCOL_ERROR = 0x40,
	EG_PROTOCOL_ERROR_PACKET = 0x41,
	EG_PROTOCOL_ERROR_COMMAND = 0x42,
	EG_PROTOCOL_ERROR_ACCESS = 0x43,
	EG_PROTOCOL_ERROR_AUTH = 0x44,
	EG_PROTOCOL_ERROR_PING = 0x45,
	EG_PROTOCOL_ERROR_STAT = 0x46,
	EG_PROTOCOL_ERROR_FLUSH = 0x47,
	EG_PROTOCOL_ERROR_USER_CREATE = 0x50,
	EG_PROTOCOL_ERROR_USER_LIST = 0x51,
	EG_PROTOCOL_ERROR_USER_RENAME = 0x52,
	EG_PROTOCOL_ERROR_USER_SET_PERM = 0x53,
	EG_PROTOCOL_ERROR_USER_DELETE = 0x54,
	EG_PROTOCOL_ERROR_QUEUE_CREATE = 0x60,
	EG_PROTOCOL_ERROR_QUEUE_DECLARE = 0x61,
	EG_PROTOCOL_ERROR_QUEUE_EXIST = 0x62,
	EG_PROTOCOL_ERROR_QUEUE_LIST = 0x63,
	EG_PROTOCOL_ERROR_QUEUE_SIZE = 0x64,
	EG_PROTOCOL_ERROR_QUEUE_PUSH = 0x65,
	EG_PROTOCOL_ERROR_QUEUE_GET = 0x66,
	EG_PROTOCOL_ERROR_QUEUE_POP = 0x67,
	EG_PROTOCOL_ERROR_QUEUE_SUBSCRIBE = 0x68,
	EG_PROTOCOL_ERROR_QUEUE_UNSUBSCRIBE = 0x69,
	EG_PROTOCOL_ERROR_QUEUE_PURGE = 0x70,
	EG_PROTOCOL_ERROR_QUEUE_DELETE = 0x71
} ProtocolResponseStatus;

typedef enum ProtocolEventType {
	EG_PROTOCOL_EVENT_NOTIFY = 0x1,
	EG_PROTOCOL_EVENT_MESSAGE = 0x2
} ProtocolEventType;

#pragma pack(push, 1)

typedef struct ProtocolRequestHeader {
	uint16_t magic;
	uint8_t cmd;
	uint8_t reserved;
	uint32_t bodylen;
} ProtocolRequestHeader;

typedef struct ProtocolResponseHeader {
	uint16_t magic;
	uint8_t cmd;
	uint8_t status;
	uint32_t bodylen;
} ProtocolResponseHeader;

typedef struct ProtocolEventHeader {
	uint16_t magic;
	uint8_t cmd;
	uint8_t type;
	uint32_t bodylen;
} ProtocolEventHeader;

typedef struct ProtocolRequestAuth {
	ProtocolRequestHeader header;
	struct {
		char name[32];
		char password[32];
	} body;
} ProtocolRequestAuth;

typedef ProtocolRequestHeader ProtocolRequestPing;
typedef ProtocolRequestHeader ProtocolRequestStat;

typedef struct ProtocolRequestFlush {
	ProtocolRequestHeader header;
	struct {
		uint32_t flags;
	} body;
} ProtocolRequestFlush;

typedef ProtocolRequestHeader ProtocolRequestDisconnect;

typedef struct ProtocolRequestUserCreate {
	ProtocolRequestHeader header;
	struct {
		char name[32];
		char password[32];
		uint64_t perm;
	} body;
} ProtocolRequestUserCreate;

typedef ProtocolRequestHeader ProtocolRequestUserList;

typedef struct ProtocolRequestUserRename {
	ProtocolRequestHeader header;
	struct {
		char from[32];
		char to[32];
	} body;
} ProtocolRequestUserRename;

typedef struct ProtocolRequestUserSetPerm {
	ProtocolRequestHeader header;
	struct {
		char name[32];
		uint64_t perm;
	} body;
} ProtocolRequestUserSetPerm;

typedef struct ProtocolRequestUserDelete {
	ProtocolRequestHeader header;
	struct {
		char name[32];
	} body;
} ProtocolRequestUserDelete;

typedef struct ProtocolRequestQueueCreate {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		uint32_t max_msg;
		uint32_t max_msg_size;
		uint32_t flags;
	} body;
} ProtocolRequestQueueCreate;

typedef struct ProtocolRequestQueueDeclare {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueDeclare;

typedef struct ProtocolRequestQueueExist {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueExist;

typedef ProtocolRequestHeader ProtocolRequestQueueList;

typedef struct ProtocolRequestQueueSize {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueSize;

typedef struct ProtocolRequestQueueGet {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueGet;

typedef struct ProtocolRequestQueuePop {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueuePop;

typedef struct ProtocolRequestQueueSubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		uint32_t flags;
	} body;
} ProtocolRequestQueueSubscribe;

typedef struct ProtocolRequestQueueUnsubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueUnsubscribe;

typedef struct ProtocolRequestQueuePurge {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueuePurge;

typedef struct ProtocolRequestQueueDelete {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestQueueDelete;

typedef struct ProtocolResponseStat {
	ProtocolResponseHeader header;
	struct {
		struct {
			uint8_t major;
			uint8_t minor;
			uint8_t patch;
		} version;
		uint32_t uptime;
		float used_cpu_sys;
		float used_cpu_user;
		uint32_t used_memory;
		uint32_t used_memory_rss;
		float fragmentation_ratio;
		uint32_t clients;
		uint32_t users;
		uint32_t queues;
		uint32_t resv1;
		uint32_t resv2;
		uint32_t resv3;
		uint32_t resv4;
	} body;
} ProtocolResponseStat;

typedef struct ProtocolResponseQueueSize {
	ProtocolResponseHeader header;
	struct {
		uint32_t size;
	} body;
} ProtocolResponseQueueSize;

typedef struct ProtocolEventQueueNotify {
	ProtocolEventHeader header;
	struct {
		char name[64];
	} body;
} ProtocolEventQueueNotify;

#pragma pack(pop)

#endif

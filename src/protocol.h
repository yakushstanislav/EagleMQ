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
#define EG_FLUSH_ROUTE_FLAG 2
#define EG_FLUSH_CHANNEL_FLAG 3

#define EG_QUEUE_AUTODELETE_FLAG 0
#define EG_QUEUE_FORCE_PUSH_FLAG 1
#define EG_QUEUE_ROUND_ROBIN_FLAG 2
#define EG_QUEUE_DURABLE_FLAG 3

#define EG_QUEUE_CLIENT_NOTIFY_FLAG 0

#define EG_ROUTE_AUTODELETE_FLAG 0
#define EG_ROUTE_ROUND_ROBIN_FLAG 1
#define EG_ROUTE_DURABLE_FLAG 2

#define EG_CHANNEL_AUTODELETE_FLAG 0
#define EG_CHANNEL_ROUND_ROBIN_FLAG 1
#define EG_CHANNEL_DURABLE_FLAG 2

typedef enum ProtocolBinaryMagic {
	EG_PROTOCOL_REQ = 0x70,
	EG_PROTOCOL_RES = 0x80,
	EG_PROTOCOL_EVENT = 0x90
} ProtocolBinaryMagic;

typedef enum ProtocolCommand {
	/* system commands (10..15) */
	EG_PROTOCOL_CMD_AUTH = 0xA,
	EG_PROTOCOL_CMD_PING = 0xB,
	EG_PROTOCOL_CMD_STAT = 0xC,
	EG_PROTOCOL_CMD_SAVE = 0xD,
	EG_PROTOCOL_CMD_FLUSH = 0xE,
	EG_PROTOCOL_CMD_DISCONNECT = 0xF,

	/* user control commands (30..34) */
	EG_PROTOCOL_CMD_USER_CREATE = 0x1E,
	EG_PROTOCOL_CMD_USER_LIST = 0x1F,
	EG_PROTOCOL_CMD_USER_RENAME = 0x20,
	EG_PROTOCOL_CMD_USER_SET_PERM = 0x21,
	EG_PROTOCOL_CMD_USER_DELETE = 0x22,

	/* queue commands (35..48) */
	EG_PROTOCOL_CMD_QUEUE_CREATE = 0x23,
	EG_PROTOCOL_CMD_QUEUE_DECLARE = 0x24,
	EG_PROTOCOL_CMD_QUEUE_EXIST = 0x25,
	EG_PROTOCOL_CMD_QUEUE_LIST = 0x26,
	EG_PROTOCOL_CMD_QUEUE_RENAME = 0x27,
	EG_PROTOCOL_CMD_QUEUE_SIZE = 0x28,
	EG_PROTOCOL_CMD_QUEUE_PUSH = 0x29,
	EG_PROTOCOL_CMD_QUEUE_GET = 0x2A,
	EG_PROTOCOL_CMD_QUEUE_POP = 0x2B,
	EG_PROTOCOL_CMD_QUEUE_CONFIRM = 0x2C,
	EG_PROTOCOL_CMD_QUEUE_SUBSCRIBE = 0x2D,
	EG_PROTOCOL_CMD_QUEUE_UNSUBSCRIBE = 0x2E,
	EG_PROTOCOL_CMD_QUEUE_PURGE = 0x2F,
	EG_PROTOCOL_CMD_QUEUE_DELETE = 0x30,

	/* route commands (49..57) */
	EG_PROTOCOL_CMD_ROUTE_CREATE = 0x31,
	EG_PROTOCOL_CMD_ROUTE_EXIST = 0x32,
	EG_PROTOCOL_CMD_ROUTE_LIST = 0x33,
	EG_PROTOCOL_CMD_ROUTE_KEYS = 0x34,
	EG_PROTOCOL_CMD_ROUTE_RENAME = 0x35,
	EG_PROTOCOL_CMD_ROUTE_BIND = 0x36,
	EG_PROTOCOL_CMD_ROUTE_UNBIND = 0x37,
	EG_PROTOCOL_CMD_ROUTE_PUSH = 0x38,
	EG_PROTOCOL_CMD_ROUTE_DELETE = 0x39,

	/* channel commands (58..67) */
	EG_PROTOCOL_CMD_CHANNEL_CREATE = 0x3A,
	EG_PROTOCOL_CMD_CHANNEL_EXIST = 0x3B,
	EG_PROTOCOL_CMD_CHANNEL_LIST = 0x3C,
	EG_PROTOCOL_CMD_CHANNEL_RENAME = 0x3D,
	EG_PROTOCOL_CMD_CHANNEL_PUBLISH = 0x3E,
	EG_PROTOCOL_CMD_CHANNEL_SUBSCRIBE = 0x3F,
	EG_PROTOCOL_CMD_CHANNEL_PSUBSCRIBE = 0x40,
	EG_PROTOCOL_CMD_CHANNEL_UNSUBSCRIBE = 0x41,
	EG_PROTOCOL_CMD_CHANNEL_PUNSUBSCRIBE = 0x42,
	EG_PROTOCOL_CMD_CHANNEL_DELETE = 0x43
} ProtocolCommand;

typedef enum ProtocolResponseStatus {
	EG_PROTOCOL_STATUS_SUCCESS = 0x1,
	EG_PROTOCOL_STATUS_ERROR = 0x2,
	EG_PROTOCOL_STATUS_ERROR_PACKET = 0x3,
	EG_PROTOCOL_STATUS_ERROR_COMMAND = 0x4,
	EG_PROTOCOL_STATUS_ERROR_ACCESS = 0x5,
	EG_PROTOCOL_STATUS_ERROR_MEMORY = 0x6,
	EG_PROTOCOL_STATUS_ERROR_VALUE = 0x7,
	EG_PROTOCOL_STATUS_ERROR_NOT_DECLARED = 0x8,
	EG_PROTOCOL_STATUS_ERROR_NOT_FOUND = 0x9,
	EG_PROTOCOL_STATUS_ERROR_NO_DATA = 0xA
} ProtocolResponseStatus;

typedef enum ProtocolEventType {
	EG_PROTOCOL_EVENT_NOTIFY = 0x1,
	EG_PROTOCOL_EVENT_MESSAGE = 0x2
} ProtocolEventType;

#pragma pack(push, 1)

typedef struct ProtocolRequestHeader {
	uint16_t magic;
	uint8_t cmd;
	uint8_t noack;
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

typedef struct ProtocolRequestSave {
	ProtocolRequestHeader header;
	struct {
		uint8_t async;
	} body;
} ProtocolRequestSave;

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

typedef struct ProtocolRequestQueueRename {
	ProtocolRequestHeader header;
	struct {
		char from[64];
		char to[64];
	} body;
} ProtocolRequestQueueRename;

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
		uint32_t timeout;
	} body;
} ProtocolRequestQueuePop;

typedef struct ProtocolRequestQueueConfirm {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		uint64_t tag;
	} body;
} ProtocolRequestQueueConfirm;

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

typedef struct ProtocolRequestRouteCreate {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		uint32_t flags;
	} body;
} ProtocolRequestRouteCreate;

typedef struct ProtocolRequestRouteExist {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestRouteExist;

typedef struct ProtocolRequestHeader ProtocolRequestRouteList;

typedef struct ProtocolRequestRouteKeys {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestRouteKeys;

typedef struct ProtocolRequestRouteRename {
	ProtocolRequestHeader header;
	struct {
		char from[64];
		char to[64];
	} body;
} ProtocolRequestRouteRename;

typedef struct ProtocolRequestRouteBind {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char queue[64];
		char key[32];
	} body;
} ProtocolRequestRouteBind;

typedef struct ProtocolRequestRouteUnbind {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char queue[64];
		char key[32];
	} body;
} ProtocolRequestRouteUnbind;

typedef struct ProtocolRequestRoutePush {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char key[32];
	} body;
} ProtocolRequestRoutePush;

typedef struct ProtocolRequestRouteDelete {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestRouteDelete;

typedef struct ProtocolRequestChannelCreate {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		uint32_t flags;
	} body;
} ProtocolRequestChannelCreate;

typedef struct ProtocolRequestChannelExist {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestChannelExist;

typedef struct ProtocolRequestHeader ProtocolRequestChannelList;

typedef struct ProtocolRequestChannelRename {
	ProtocolRequestHeader header;
	struct {
		char from[64];
		char to[64];
	} body;
} ProtocolRequestChannelRename;

typedef struct ProtocolRequestChannelPublish {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char topic[32];
	} body;
} ProtocolRequestChannelPublish;

typedef struct ProtocolRequestChannelSubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char topic[32];
	} body;
} ProtocolRequestChannelSubscribe;

typedef struct ProtocolRequestChannelPatternSubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char pattern[32];
	} body;
} ProtocolRequestChannelPatternSubscribe;

typedef struct ProtocolRequestChannelUnsubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char topic[32];
	} body;
} ProtocolRequestChannelUnsubscribe;

typedef struct ProtocolRequestChannelPatternUnsubscribe {
	ProtocolRequestHeader header;
	struct {
		char name[64];
		char pattern[32];
	} body;
} ProtocolRequestChannelPatternUnsubscribe;

typedef struct ProtocolRequestChannelDelete {
	ProtocolRequestHeader header;
	struct {
		char name[64];
	} body;
} ProtocolRequestChannelDelete;

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
		uint32_t routes;
		uint32_t channels;
		uint32_t resv3;
		uint32_t resv4;
	} body;
} ProtocolResponseStat;

typedef struct ProtocolResponseQueueExist {
	ProtocolResponseHeader header;
	struct {
		uint32_t status;
	} body;
} ProtocolResponseQueueExist;

typedef struct ProtocolResponseQueueSize {
	ProtocolResponseHeader header;
	struct {
		uint32_t size;
	} body;
} ProtocolResponseQueueSize;

typedef struct ProtocolResponseRouteExist {
	ProtocolResponseHeader header;
	struct {
		uint32_t status;
	} body;
} ProtocolResponseRouteExist;

typedef struct ProtocolResponseChannelExist {
	ProtocolResponseHeader header;
	struct {
		uint32_t status;
	} body;
} ProtocolResponseChannelExist;

typedef struct ProtocolEventQueueNotify {
	ProtocolEventHeader header;
	struct {
		char name[64];
	} body;
} ProtocolEventQueueNotify;

#pragma pack(pop)

#endif

/**************************************************************
 * Description: SyncScribe library to manage network and local events,
 * variables and channels
 * Copyright (c) 2022 Alexander Krapivniy (a.krapivniy@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************/

#ifndef __SYNCS_TYPES__
#define __SYNCS_TYPES__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <netinet/in.h>

#define SYNCS_DEFAULT_SYNC_OFFSET_MS	 300
#define SYNCS_EVENT_MAXIMUM		 256
#define SYNCS_VARIABLE_SIZE_MAXIMUM	 0x1ff // using for bit mask, should be 2^n-1 (511 because max UDP size + header )
#define SYNCS_CHANNEL_MESSAGE_SIZE	 SYNCS_VARIABLE_SIZE_MAXIMUM
#define SYNCS_EVENT_DATA_SIZE_MAXIMUM	 0xfff
#define SYNCS_VARIABLE_INFO_SIZE_MAXIMUM 32
#define SYNCS_CLIENT_MAXIMUM		 64
#define SYNCS_CHANNEL_MAXIMUM		 32
#define SYNCS_EVENT_NAME_SIZE		 32
#define SYNCS_CLIENT_BUFFER_SIZE	 (32*1024)
#define SYNCS_CRYPT_KEY_SIZE		 32

union syncs_id {
	uint64_t i[SYNCS_EVENT_NAME_SIZE / sizeof(uint64_t)];
	char c[SYNCS_EVENT_NAME_SIZE];
} __attribute__((packed));

typedef union syncs_id syncsid_t;

#define SYNCS_PACKET_MAGIC	('S')
#define SYNCS_PACKET_MAGIC_DATA ('D')
#define UDP_SOCKET_STUB		(-2)
#define SYNCS_VERSION_MAJOR	2
#define SYNCS_VERSION_MINOR	1

struct syncdata{
	uint32_t data0;
	uint32_t data1;
} __attribute__((packed));

//Bitmap of data_size:
//	00..11 - data size (0..4095)
//	12..15 - data padding 0..15

struct syncs_header {
	uint8_t magic;
	uint32_t crc;
	uint32_t type;
	syncsid_t id;
	struct syncdata sync;
	uint64_t update_counter;
	uint16_t data_size;
	uint8_t magic_data;
} __attribute__((packed));

struct syncs_packet {
	struct syncs_header header;
	char buffer[SYNCS_EVENT_DATA_SIZE_MAXIMUM];
} __attribute__((packed));

struct syncs_client_id {
	uint32_t version;
	syncsid_t groupid;
} __attribute__((packed));

struct syncs_event_info {
	syncsid_t id;
	uint32_t type;
	uint8_t short_data[32];
	uint16_t data_size;
	time_t time;
	uint32_t count;
	uint32_t consumers_count;
	uint32_t producers_count;
} __attribute__((packed));

struct syncs_client_info {
	syncsid_t id;
	uint32_t event_subscribe;
	uint32_t event_write;
	uint32_t rx_event_count;
	uint32_t tx_event_count;
	in_addr_t ip;
} __attribute__((packed));

struct syncs_channel_info {
	syncsid_t id;
	uint32_t anons_count;
	uint32_t request_count;
	in_addr_t ip;
	uint16_t port;
} __attribute__((packed));

struct syncs_channel_ticket {
	in_addr_t ip;
	uint16_t port;
	uint32_t flags;
} __attribute__((packed));

#define SYNCS_PACKET_SIZE_MASK (0x0fff)
#define SYNCS_PACKET_SIZE_PADDING(data_size) ((data_size)>>12)
#define SYNCS_PACKET_DATA_SIZE(data_size) ((data_size)&SYNCS_PACKET_SIZE_MASK)
#define SYNCS_SET_PACKET_SIZE(size, pad) (((pad) << 12) | SYNCS_PACKET_DATA_SIZE(size))

#define SYNCS_PACKET_SIZE_HEAD(packet_header) (sizeof(struct syncs_header) + (((packet_header)->data_size)&SYNCS_PACKET_SIZE_MASK))
#define SYNCS_PACKET_SIZE(packet) (sizeof(struct syncs_header) + (SYNCS_PACKET_DATA_SIZE(((packet)->header.data_size))))
#define SYNCS_CRYPT_HEADER_SIZE	 (sizeof(struct syncs_header) - 1 - 2 - 1)
#define SYNCS_CRC_HEADER_SIZE	 (sizeof(struct syncs_header) - 1 - 4)

#define SYNCS_CLIENT_MODE_TCP	    0x00000001
#define SYNCS_CLIENT_MODE_UDP	    0x00000002
#define SYNCS_CLIENT_MODE_ICMP	    0x00000004
#define SYNCS_CLIENT_MODE_BROADCAST 0x00000008

// EVENT TYPE FLAG BIT MAP
// 0..3 - event message type
// 4..7 - event message bit attributes
// 8..11 - data type / channel type
// 12..15 - data end attribute bit

// 28..31 - writer ID

#define SYNCS_TYPE_EMPTY	(0x00)
#define SYNCS_TYPE_CLIENT_ID	(0x01)
#define SYNCS_TYPE_EVENT	(0x02)
#define SYNCS_TYPE_WRITE	(0x03)
#define SYNCS_TYPE_READ		(0x04)
#define SYNCS_TYPE_SUBSCRIBE	(0x05)
#define SYNCS_TYPE_UNSUBSCRIBE	(0x06)
#define SYNCS_TYPE_DEFINE	(0x07)
#define SYNCS_TYPE_UNDEFINE	(0x08)
#define SYNCS_TYPE_EVENT_LIST	(0x09)
#define SYNCS_TYPE_CLIENT_LIST	(0x0a)
#define SYNCS_TYPE_SERVER_STATUS (0x0b)
#define SYNCS_TYPE_ACK		(0x0c)
#define SYNCS_TYPE_CHANNEL	(0x0d)
#define SYNCS_TYPE_CHANNEL_LIST (0x0e)
#define SYNCS_TYPE_RESERVED	(0x0f)
#define SYNCS_TYPE_MSG_MASK	(0x0f)

#define SYNCS_TYPE_SYNC	      (0x0010)
#define SYNCS_TYPE_ECHO	      (0x0020)
#define SYNCS_TYPE_CRYPT      (0x0040)
#define SYNCS_TYPE_FORCE      (0x0080)
#define SYNCS_TYPE_FLAGS_MASK (0x00f0)

#define SYNCS_TYPE_VAR_NOT_DEFINED (0x0000)
#define SYNCS_TYPE_VAR_EMPTY	   (0x0100)
#define SYNCS_TYPE_VAR_INT32	   (0x0200)
#define SYNCS_TYPE_VAR_INT64	   (0x0300)
#define SYNCS_TYPE_VAR_FLOAT	   (0x0400)
#define SYNCS_TYPE_VAR_DOUBLE	   (0x0500)
#define SYNCS_TYPE_VAR_STRING	   (0x0600)
#define SYNCS_TYPE_VAR_STRUCTURE   (0x0700)
#define SYNCS_TYPE_VAR_STREAM	   (0x0800)
#define SYNCS_TYPE_VAR_HUGE	   (0x0d00)
#define SYNCS_TYPE_VAR_RESERVED	   (0x0e00)
#define SYNCS_TYPE_VAR_ANY	   (0x0f00)
#define SYNCS_TYPE_VAR_MASK	   (0x0f00)

#define SYNCS_TYPE_CHANNEL_UDP	(0x0100)
#define SYNCS_TYPE_CHANNEL_TCP	(0x0200)
#define SYNCS_TYPE_CHANNEL_ICMP	(0x0300)
#define SYNCS_TYPE_CHANNEL_MASK (0x0700)
#define SYNCS_TYPE_CHANNEL_FORWARD	(0x0F00)

#define SYNCS_CHANNEL_ANONS   (0x1000)
#define SYNCS_CHANNEL_REQUEST (0x2000)
#define SYNCS_CHANNEL_TICKET  (0x3000)
#define SYNCS_CHANNEL_MASK    (0xf000)

#define SYNCS_STATUS_LOST	(0x10000000)
#define SYNCS_STATUS_CRYPT	(0x80000000)
#define SYNCS_STATUS_FLAGS_MASK (0xf0000000)

#define SYNCS_WRITER_ID_MASK	(0x00ff0000)

#define SYNCS_CLIENT_TYPE_ENDPOINT	(0x0001)
#define SYNCS_CLIENT_TYPE_SERVER	(0x0002)
#define SYNCS_CLIENT_TYPE_MASK		(0x000f)

#define SYNCS_ERROR_NOTFOUND	  0
#define SYNCS_ERROR_NOTSUPPORT	  1
#define SYNCS_ERROR_UNKNOWNCLIENT 2
#define SYNCS_ERROR_CRYPT         3

#ifdef __cplusplus
}
#endif

#endif //__SYNCS_TYPES__

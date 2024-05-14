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

#ifndef __SYNCS_COMMON__
#define __SYNCS_COMMON__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "syncs-types.h"


#define SSDP_PACKET_SIZE 1500

static struct {
	const char *msearch;
	const char *notify;
	const char *response;
} ssdp_headers = {
	.msearch = "M-SEARCH * HTTP/1.1\r\n",
	.notify = "NOTIFY * HTTP/1.1\r\n",
	.response = "HTTP/1.1 200 OK\r\n",
};

static struct {
	const char *ip;
	const int port;
} ssdp_network = {
	.ip = "239.255.255.250",
	.port = 1900,
};

static struct {
	const char *name;
} syncs_ssdp_field = {
	.name = "syncscribe-server",
};

#define SYNCS_COMPARE_ID(a,b) ((a[0]==b[0])&&(a[1]==b[1])&&(a[2]==b[2])&&(a[3]==b[3]))

static __attribute__((always_inline)) inline int syncs_idcmp (syncsid_t *a, syncsid_t *b)
{
	if (SYNCS_COMPARE_ID (a->i,b->i)) return 1; else return 0;
}

static __attribute__((always_inline)) inline int syncs_idcmp_str (syncsid_t *a, const char *b)
{
	char *s = a->c;
	int i = sizeof (syncsid_t);
	while (--i && *b)
		if (*s++ != *b++)
			return 0;
	return (*s == *b)? 1 : 0;
}

static __attribute__((always_inline)) inline void syncs_idcpy (syncsid_t *a, syncsid_t *b)
{
	a->i[0] = b->i[0];
	a->i[1] = b->i[1];
	a->i[2] = b->i[2];
	a->i[3] = b->i[3];
}

static __attribute__((always_inline)) inline void syncs_idstr (syncsid_t *a, const char *b)
{
	char *s = a->c;
	int i = sizeof (syncsid_t);
	a->i[0] = 0; a->i[1] = 0; a->i[2] = 0; a->i[3] = 0;
	while (--i && (*s++ = *b++ )!= 0);
}

static __attribute__((always_inline)) inline void syncs_stridcpy (char *a, const syncsid_t *b)
{
	const char *s = b->c;
	int i = sizeof (syncsid_t);
	while (--i && (*a++ = *s++ )!= 0);
}

static __attribute__((always_inline)) inline uint32_t syncs_get_size_by_type(uint32_t type)
{
	switch (type & SYNCS_TYPE_VAR_MASK) {
		case SYNCS_TYPE_VAR_FLOAT:
		case SYNCS_TYPE_VAR_INT32: return 4;
		case SYNCS_TYPE_VAR_DOUBLE:
		case SYNCS_TYPE_VAR_INT64: return 8;
	}
	return 0;
}

static __attribute__((always_inline)) inline void syncs_fill_basic_header (struct syncs_header *p, uint32_t type)
{
        p->magic = SYNCS_PACKET_MAGIC;
        p->magic_data = SYNCS_PACKET_MAGIC_DATA;
        p->type = type;
}

static __attribute__((always_inline)) inline void syncs_fill_basic_header_request (struct syncs_header *p, uint32_t type)
{
	syncs_fill_basic_header(p, type);
	p->data_size = 0;
}

static __attribute__((always_inline)) inline void syncs_fill_header (struct syncs_header *p, syncsid_t *id, uint32_t type)
{
	syncs_fill_basic_header(p, type);
        syncs_idcpy(&p->id, id);
}

static __attribute__((always_inline)) inline void syncs_fill_header_str (struct syncs_header *p, const char *cid, uint32_t type)
{
	syncs_fill_basic_header(p, type);
        syncs_idstr(&p->id, cid);
}

static __attribute__((always_inline)) inline void syncs_fill_header_request_str (struct syncs_header *p, const char *cid, uint32_t type)
{
	syncs_fill_basic_header_request(p, type);
        syncs_idstr(&p->id, cid);
}

static __attribute__((always_inline)) inline void syncs_fill_header_request_id (struct syncs_header *p, syncsid_t *id, uint32_t type)
{
	syncs_fill_basic_header_request(p, type);
        syncs_idcpy(&p->id, id);
}

#ifdef __cplusplus
}
#endif



#endif //__SYNCS_COMMON__

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "syncs-net.h"
#include "syncs-common.h"
#include "syncs-crypt.h"
#include "syncs-server-types.h"

#define MODULE_NAME "syncs-server"
#include <syncs-debug.h>
#undef syncsd_debug
#define syncsd_debug(fmt,args...)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

struct syncs_client *syncs_get_free_client(struct syncs_server *s)
{
	int i;
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++)
		if (s->clients[i].socketfd == -1)
			return(&s->clients[i]);
	syncsd_error("havn't space for new client");
	return NULL;
}

void syncs_event_init(struct syncs_event *event, syncsid_t *id)
{
	int i;
	event->args = NULL;
	event->cb = NULL;
	event->count = 0;
	event->data[0] = 0;
	event->data_size = 0;
	event->data_type = 0;
	event->producer = NULL;
	event->update_counter = 0;
	event->consumers_count = 0;
	event->producers_count = 0;

	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		event->consumers[i] = NULL;
	}
	syncs_idcpy(&event->id, id);
}

void syncs_channel_init(struct syncs_channel *channel, syncsid_t *id)
{
	channel->producer = NULL;
	channel->anons_count = 0;
	channel->producers_count = 0;
	channel->request_count = 0;

	syncs_idcpy(&channel->id, id);
}

struct syncs_channel *syncs_create_channel(struct syncs_server *s, syncsid_t *id)
{
	int i;

	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++)
		if (s->channels[i].id.i[0] == -1) {
			syncs_channel_init(&s->channels[i], id);
			s->channel_count++;
			return &s->channels[i];
		}
	syncsd_error("havn't space for create channel");
	return NULL;
}

struct syncs_event *syncs_create_event(struct syncs_server *s, syncsid_t *id)
{
	int i;

	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (s->events[i].id.i[0] == -1) {
			syncs_event_init(&s->events[i], id);
			s->event_count++;
			return &s->events[i];
		}
	syncsd_error("haven't space for create event");
	return NULL;
}

struct syncs_event *syncs_find_event(struct syncs_server *s, syncsid_t *id)
{
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (syncs_idcmp(&s->events[i].id, id)) {
			return &s->events[i];
		}
	return NULL;
}

struct syncs_client *syncs_find_uclient_id(struct syncs_server *s, syncsid_t *id)
{
	int i;
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		if (s->clients[i].socketfd != UDP_SOCKET_STUB) continue;
		if (syncs_idcmp(&s->clients[i].id, id)) {
			return &s->clients[i];
		}
	}
	return NULL;
}

struct syncs_client *syncs_find_uclient_addr(struct syncs_server *s, struct sockaddr_in *addr)
{
	int i;
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		if (s->clients[i].socketfd != UDP_SOCKET_STUB) continue;
		if (s->clients[i].addr.sin_addr.s_addr != addr->sin_addr.s_addr)
			continue;
		if (s->clients[i].addr.sin_port != addr->sin_port)
			continue;
		return &s->clients[i];
	}
	return NULL;
}

struct syncs_channel *syncs_find_channel(struct syncs_server *s, syncsid_t *id)
{
	int i;
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++)
		if (syncs_idcmp(&s->channels[i].id, id)) {
			return &s->channels[i];
		}
	return NULL;
}

void syncs_free_event(struct syncs_server *s, syncsid_t *id)
{
	struct syncs_event *event = syncs_find_event(s, id);

	if (event != NULL) {
		event->id.i[0] = -1;
		s->event_count--;
	}
}

void syncs_free_channel(struct syncs_server *s, syncsid_t *id)
{
	struct syncs_channel *channel = syncs_find_channel(s, id);

	if (channel != NULL) {
		channel->id.i[0] = -1;
		s->channel_count--;
	}
}

static int syncs_client_send(struct syncs_client *c, struct syncs_packet *packet)
{
	void *buffer;
	uint32_t size;

	//TODO: add packet encoder
	buffer = packet;
	size = SYNCS_PACKET_SIZE(packet);

	if (c->socketfd > -1) {
		return(syncs_blocking_send(c->socketfd, buffer, size, MSG_NOSIGNAL));
	} else if (c->socketfd == UDP_SOCKET_STUB) {
		syncs_udp_send(c->server->usocketfd, buffer, size, &c->addr, c->addr_size);
		return 0;
	}
	return -1;
}

int syncs_add_client_to_event(struct syncs_client *c, struct syncs_event *event, struct syncs_client *consumers[])
{
	int i;

	syncsd_debug("add client to event");
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		if (consumers[i] == c) {
			syncsd_error("already subscribes, skip");
			return 0;
		}
	}
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		syncsd_debug("active client %p in %d", event->consumers[i], i);
		if (consumers[i] == NULL) {
			syncsd_debug("add client %p in %d", c, i);
			consumers[i] = c;
			event->consumers_count++;
			c->event_subscribe++;
			return 0;
		}
	}
	return -1;
}

void syncs_remove_client_from_event(struct syncs_client *c, struct syncs_event *event)
{
	int i;

	syncsd_debug("looking for client %p in event %s", c, event->id.c);
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		syncsd_debug("active client %p in %d", event->consumers[i], i);
		if (event->consumers[i] == c) {
			event->consumers[i] = NULL;
			event->consumers_count--;
			c->event_subscribe--;
			return;
		}
	}
	return;
}

void syncs_remove_client_from_events(struct syncs_client *c)
{
	struct syncs_server *s = c->server;
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++) {
		if (s->events[i].id.i[0] != -1) {
			syncs_remove_client_from_event(c, &s->events[i]);
		}
	}
}

void syncs_remove_channels_of_client(struct syncs_client *c)
{
	struct syncs_server *s = c->server;
	int i;
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++) {
		if (s->channels[i].id.i[0] != -1) {
			if (s->channels[i].producer == c)
				s->channels[i].id.i[0] = -1;
		}
	}
}

int syncs_server_subscribe_event(struct syncs_server *s, uint32_t flags, const char *cid, void (*cb)(void *, char *, void *, uint32_t), void *args)
{
	syncsid_t id;
	struct syncs_event *event;

	syncs_idstr(&id, cid);
	event = syncs_find_event(s, &id);

	if (event == NULL) {
		if (!(flags & SYNCS_TYPE_FORCE)) return -1;
		event = syncs_create_event(s, &id);
		if (event == NULL) return -2;
	}

	event->args = args;
	event->cb = cb;

	return 0;
}

void syncs_server_unsubscribe_event(struct syncs_server *s, const char *cid)
{
	syncsid_t id;
	struct syncs_event *event;

	syncs_idstr(&id, cid);
	event = syncs_find_event(s, &id);

	if (event == NULL) return;

	event->cb = NULL;
	event->args = NULL;
}

void syncs_sync_calculate(int offset_s, int offset_ms, struct syncdata *sync)
{
	struct timespec sync_time;

	clock_gettime(CLOCK_REALTIME, &sync_time);
	sync_time.tv_sec += offset_s;
	sync_time.tv_nsec += offset_ms * 1000000L;
	if (sync_time.tv_nsec > 1000000000L) {
		sync_time.tv_nsec -= 1000000000L;
		sync_time.tv_sec++;
	}
	sync->data0 = sync_time.tv_sec;
	sync->data1 = sync_time.tv_nsec;
}

int syncs_send_event(struct syncs_server *s, struct syncs_event *event, int flags)
{
	struct syncs_packet packet;
 	struct syncs_client *c;
	int i;

	syncs_fill_header(&packet.header, &event->id, SYNCS_TYPE_EVENT | event->data_type);
	memcpy(packet.buffer, event->data, event->data_size);
	packet.header.data_size = event->data_size;

	if (flags & SYNCS_TYPE_SYNC) {
		packet.header.type |= SYNCS_TYPE_SYNC;
		syncs_sync_calculate(0, s->sync_offset, &packet.header.sync);
	}

	syncsd_debug("send event %s", &event->id.c[0]);
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		c = event->consumers[i];
		if (c == NULL) continue;
		if (c == event->producer && !(flags & SYNCS_TYPE_ECHO)) continue;
		c->tx_event_count++;
		syncsd_debug("send event for %s", &event->id.c[0]);
		if (syncs_client_send(c, &packet)) {
			c->tx_error++;
		}
	}
	return 0;
}

int syncs_resend_event(struct syncs_client *c, struct syncs_event * event)
{
	struct syncs_packet packet;

	syncs_fill_header(&packet.header, &event->id, SYNCS_TYPE_EVENT | SYNCS_STATUS_LOST);
	memcpy(packet.buffer, event->data, event->data_size);
	packet.header.data_size = event->data_size;

	syncs_client_send(c, &packet);
	c->tx_event_count++;

	return 0;
}

int syncs_send_server_status(struct syncs_client *c, int code)
{
	struct syncs_packet packet;

	syncs_fill_header_request_str(&packet.header, c->server->id.c, SYNCS_TYPE_SERVER_STATUS);
	packet.header.update_counter = code;

	syncs_client_send(c, &packet);
	c->tx_event_count++;
	return 0;
}

int syncs_send_udp_server_status(struct syncs_server *s, struct sockaddr_in *addr, int code)
{
	struct syncs_packet packet;
	int size = sizeof(struct syncs_header);

	syncs_fill_header_request_str(&packet.header, s->id.c, SYNCS_TYPE_SERVER_STATUS);
	packet.header.update_counter = code;

	sendto(s->usocketfd, &packet, size, MSG_NOSIGNAL, (struct sockaddr *) addr, sizeof(struct sockaddr_in));
	return 0;
}

int syncs_server_write(struct syncs_server *s, int flags, const char *cid, void *data, uint32_t data_size)
{
	syncsid_t id;
	struct syncs_event *event;

	syncs_idstr(&id, cid);

	syncsd_debug("writing event");

	event = syncs_find_event(s, &id);
	if (event == NULL) {
		syncsd_debug("event not found");
		if (!(flags & SYNCS_TYPE_FORCE)) return -1;
		syncsd_debug("create event");
		event = syncs_create_event(s, &id);
		if (event == NULL) return -2;
	}

	if (!data_size)
		data_size = syncs_get_size_by_type(flags);
	memcpy(event->data, data, data_size);
	event->data_size = data_size;

	if (event->producer != NULL) {
		event->producer = NULL;
		event->producers_count++;
	}
	syncsd_debug("send event");
	syncs_send_event(s, event, flags);
	return 0;
}

int syncs_server_write_int32(struct syncs_server *s, uint32_t flags, const char *id, int32_t data)
{
	return syncs_server_write(s, flags | SYNCS_TYPE_VAR_INT32, id, &data, 0);

}

int syncs_server_write_int64(struct syncs_server *s, uint32_t flags, const char *id, int64_t data)
{
	return syncs_server_write(s, flags | SYNCS_TYPE_VAR_INT64, id, &data, 0);
}

int syncs_server_write_float(struct syncs_server *s, uint32_t flags, const char *id, float data)
{
	return syncs_server_write(s, flags | SYNCS_TYPE_VAR_FLOAT, id, &data, 0);
}

int syncs_server_write_double(struct syncs_server *s, uint32_t flags, const char *id, double data)
{
	return syncs_server_write(s, flags | SYNCS_TYPE_VAR_DOUBLE, id, &data, 0);
}

int syncs_server_write_str(struct syncs_server *s, uint32_t flags, const char *id, const char *data)
{
	return syncs_server_write(s, flags | SYNCS_TYPE_VAR_STRING, id, (void *) data, strlen(data) + 1);
}

int syncs_server_read(struct syncs_server *s, uint32_t flags, const char *cid, void *data, uint32_t *data_size)
{
	syncsid_t id;
	struct syncs_event *event;

	syncs_idstr(&id, cid);
	event = syncs_find_event(s, &id);

	if (event == NULL)
		return -1;

	if (*data_size > event->data_size)
		*data_size = event->data_size;
	memcpy(data, event->data, *data_size);

	return 0;
}

int syncs_server_read_int32(struct syncs_server *s, uint32_t flags, const char *id, int32_t *data)
{
	uint32_t size = sizeof(int);
	return(syncs_server_read(s, flags | SYNCS_TYPE_VAR_INT32, id, data, &size));
}

int syncs_server_read_int64(struct syncs_server *s, uint32_t flags, const char *id, int64_t *data)
{
	uint32_t size = sizeof(long);
	return(syncs_server_read(s, flags | SYNCS_TYPE_VAR_INT64, id, data, &size));
}

int syncs_server_read_float(struct syncs_server *s, uint32_t flags, const char *id, float *data)
{
	uint32_t size = sizeof(float);
	return(syncs_server_read(s, flags | SYNCS_TYPE_VAR_FLOAT, id, data, &size));
}

int syncs_server_read_double(struct syncs_server *s, uint32_t flags, const char *id, double *data)
{
	uint32_t size = sizeof(double);
	return(syncs_server_read(s, flags | SYNCS_TYPE_VAR_DOUBLE, id, data, &size));
}

int syncs_server_read_str(struct syncs_server *s, uint32_t flags, const char *id, char *data, uint32_t size)
{
	return(syncs_server_read(s, flags | SYNCS_TYPE_VAR_STRING, id, data, &size));
}

void syncs_server_cb_event(struct syncs_server *s, syncsid_t * id)
{
	void (*cb)(void *, char *, void *data, uint32_t size);
	void *args;
	struct syncs_event *event = syncs_find_event(s, id);

	if (event == NULL) return;

	cb = event->cb;
	args = event->args;
	if (cb != NULL) {
		cb(args, id->c, event->data, event->data_size);
	}
}

void syncs_close_client_socket(struct syncs_client * c)
{
	int socketfd = c->socketfd;

	epoll_ctl(c->server->epollfd, EPOLL_CTL_DEL, socketfd, NULL);

	syncs_remove_client_from_events(c);
	syncs_remove_channels_of_client(c);

	c->socketfd = -1;
	close(socketfd);
	syncsd_debug("client disconnected %d", socketfd);
}

int syncs_client_subscribe(struct syncs_client *c, syncsid_t *id, uint32_t flags, uint64_t update_counter)
{
	struct syncs_event *event;

	syncsd_debug("client subscribe");
	event = syncs_find_event(c->server, id);
	if (event == NULL) {
		syncsd_debug("event not found");
		if (!(flags & SYNCS_TYPE_FORCE)) return -1;
		event = syncs_create_event(c->server, id);
		if (event == NULL) return -2;
		event->data_type = flags & SYNCS_TYPE_VAR_MASK;
	}

	syncsd_debug("found event: type [0x%08x:0x%08x] ", flags&SYNCS_TYPE_VAR_MASK, event->data_type);

	if (((event->data_type & SYNCS_TYPE_VAR_MASK) == SYNCS_TYPE_VAR_ANY) || ((flags & SYNCS_TYPE_VAR_MASK) == SYNCS_TYPE_VAR_ANY) || ((flags & SYNCS_TYPE_VAR_MASK) == event->data_type)) {
		syncs_add_client_to_event(c, event, event->consumers);
	} else return -3;

	if (update_counter < event->update_counter)
		syncs_resend_event(c, event);
	return 0;
}

int syncs_client_unsubscribe(struct syncs_client *c, syncsid_t * id)
{
	struct syncs_event *event;

	syncsd_debug("client unsubscribe");
	event = syncs_find_event(c->server, id);
	if (event != NULL) {
		syncs_remove_client_from_event(c, event);
	}
	return 0;
}

int syncs_add_event(struct syncs_server *s, syncsid_t *id, uint32_t flags, void *data, uint32_t size)
{
	struct syncs_event *event = NULL;

	syncsd_debug("looking for exist event %s", (char *) id);
	event = syncs_find_event(s, id);

	if (event == NULL) {
		event = syncs_create_event(s, id);
		if (event == NULL) return -1;
	} else
		if (!(flags & SYNCS_TYPE_FORCE)) return -2;

	event->data_type = flags & SYNCS_TYPE_VAR_MASK;
	if ((size > 1) && (size < SYNCS_VARIABLE_SIZE_MAXIMUM))
		event->data_size = size;
	else event->data_size = syncs_get_size_by_type(flags);
	if (data != NULL)
		memcpy(event->data, data, event->data_size);

	return 0;
}

int syncs_server_define(struct syncs_server *s, const char *cid, uint32_t flags, void *data, uint32_t size)
{
	syncsid_t id;

	syncs_idstr(&id, cid);
	if (size > SYNCS_VARIABLE_SIZE_MAXIMUM) {
		syncsd_error("size of variable %s more than maximum %d", cid, SYNCS_VARIABLE_SIZE_MAXIMUM);
		return -5;
	}
	return(syncs_add_event(s, &id, flags, data, size));
}

int syncs_client_read(struct syncs_client *c, syncsid_t * id)
{
	struct syncs_event *event;
	struct syncs_packet packet;

	syncsd_debug("client wants to read event %s", (char *) id);

	syncs_fill_header(&packet.header, id, SYNCS_TYPE_READ);

	event = syncs_find_event(c->server, id);
	if (event != NULL) {
		syncsd_debug("found event %s for read", (char *) id);
		memcpy(packet.buffer, event->data, event->data_size);
		packet.header.data_size = event->data_size;
		packet.header.type |= (event->data_type & SYNCS_TYPE_VAR_MASK);
	} else {
		syncsd_debug("no event %s for read", (char *) id);
		packet.header.type |= SYNCS_TYPE_VAR_NOT_DEFINED;
		packet.header.data_size = 0;
	}
	syncs_client_send(c, &packet);
	c->tx_event_count++;
	return 0;
}

int syncs_client_write(struct syncs_client *c, syncsid_t *id, uint32_t flags, char *data, uint32_t data_size)
{
	struct syncs_event *event;
	void (*cb)(void *, char *, void *, uint32_t);
	void *args;
	struct syncs_server *s = c->server;

	syncsd_debug("write");
	event = syncs_find_event(s, id);
	if (event == NULL) {
		syncsd_debug("NULL");
		if (!(flags & SYNCS_TYPE_FORCE)) return -1;
		event = syncs_create_event(c->server, id);
		if (event == NULL) return -2;
		event->data_type = flags & SYNCS_TYPE_VAR_MASK;
	}

	if ((flags & SYNCS_TYPE_VAR_MASK) != event->data_type) {
		syncsd_debug("error type");
		return -3;
	}

	if (event->producer != c) {
		event->producer = c;
		event->producers_count++;
	}
	event->count++;
	c->event_write++;

	memcpy(event->data, data, data_size);
	event->data_size = data_size;
	event->update_counter = s->update_counter;
	s->update_counter++;
	syncsd_debug("new data = %d:%d", *(int *) event->data, event->data_size);

	cb = event->cb;
	args = event->args;
	if (cb != NULL) {
		cb(args, id->c, event->data, event->data_size);
	}

	syncs_send_event(s, event, flags & (SYNCS_TYPE_VAR_MASK | SYNCS_TYPE_SYNC | SYNCS_TYPE_ECHO));
	return 0;
}

int syncs_server_undefine(struct syncs_server *s, const char *cid)
{
	syncsid_t id;

	syncs_idstr(&id, cid);
	syncs_free_event(s, &id);
	return 0;
}

void syncs_client_send_clientlist(struct syncs_client *c, uint8_t sequance)
{
	struct syncs_server *s = c->server;
	struct syncs_packet packet;
	uint8_t max_clients_in_packet = SYNCS_VARIABLE_SIZE_MAXIMUM / sizeof(struct syncs_client_info);
	struct syncs_client_info *client_info = (struct syncs_client_info *) packet.buffer;
	uint8_t client_count = 0;
	int i;
	struct syncs_client *client;


	packet.header.magic = SYNCS_PACKET_MAGIC;
	packet.header.magic_data = SYNCS_PACKET_MAGIC_DATA;
	packet.header.type = SYNCS_TYPE_CLIENT_LIST;

	packet.header.id.c[0] = 0;
	packet.header.id.c[1] = (s->client_count / max_clients_in_packet) + 1;
	packet.header.id.c[3] = sequance;
	packet.header.id.c[4] = 0;

	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++)
		if (s->clients[i].socketfd != -1) {
			client = &s->clients[i];
			client_info[client_count].id = client->id;
			client_info[client_count].event_subscribe = client->event_subscribe;
			client_info[client_count].event_write = client->event_write;
			client_info[client_count].rx_event_count = client->rx_event_count;
			client_info[client_count].tx_event_count = client->tx_event_count;
			client_info[client_count].ip = client->addr.sin_addr.s_addr;
			client_count++;
			if (client_count >= max_clients_in_packet) {
				packet.header.id.c[2] = client_count;
				packet.header.data_size = client_count * sizeof(struct syncs_client_info);
				syncs_client_send(c, &packet);
				c->tx_event_count++;
				packet.header.id.c[0]++;
				client_count = 0;
			}
		}
	packet.header.id.c[4] = 1;
	if (client_count) {
		packet.header.id.c[2] = client_count;
		packet.header.data_size = client_count * sizeof(struct syncs_client_info);
		syncs_client_send(c, &packet);
		c->tx_event_count++;
	} else {
		packet.header.id.c[2] = 0;
		packet.header.data_size = 0;
		syncs_client_send(c, &packet);
		c->tx_event_count++;
	}
}

void syncs_client_send_eventlist(struct syncs_client *c, uint8_t sequance)
{
	struct syncs_server *s = c->server;
	struct syncs_packet packet;
	uint8_t max_events_in_packet = SYNCS_VARIABLE_SIZE_MAXIMUM / sizeof(struct syncs_event_info);
	struct syncs_event_info *event_info = (struct syncs_event_info *) packet.buffer;
	uint8_t event_count = 0;
	int i;
	struct syncs_event *event;


	packet.header.magic = SYNCS_PACKET_MAGIC;
	packet.header.magic_data = SYNCS_PACKET_MAGIC_DATA;
	packet.header.type = SYNCS_TYPE_EVENT_LIST;

	packet.header.id.c[0] = 0;
	packet.header.id.c[1] = (s->event_count / max_events_in_packet) + 1;
	packet.header.id.c[3] = sequance;
	packet.header.id.c[4] = 0;
	syncsd_debug("send event list max events in package %d, packages %d, seq %d", max_events_in_packet, packet.header.id.c[1], packet.header.id.c[3]);

	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (s->events[i].id.i[0] != -1) {
			event = &s->events[i];
			event_info[event_count].id = event->id;
			event_info[event_count].consumers_count = event->consumers_count;
			event_info[event_count].count = event->count;
			event_info[event_count].data_size = event->data_size;
			event_info[event_count].producers_count = event->producers_count;
			event_info[event_count].type = event->data_type;
			memcpy(&event_info[event_count].short_data, event->data, MIN(event->data_size, SYNCS_VARIABLE_INFO_SIZE_MAXIMUM));
			event_count++;
			if (event_count >= max_events_in_packet) {
				syncsd_debug("send events packet with %d events", event_count);
				packet.header.id.c[2] = event_count;
				packet.header.data_size = event_count * sizeof(struct syncs_event_info);
				syncs_client_send(c, &packet);
				c->tx_event_count++;
				event_count = 0;
				packet.header.id.c[0]++;
			}
		}
	packet.header.id.c[4] = 1;
	if (event_count) {
		packet.header.id.c[2] = event_count;
		packet.header.data_size = event_count * sizeof(struct syncs_event_info);
		syncs_client_send(c, &packet);
		c->tx_event_count++;
	} else {
		packet.header.id.c[2] = 0;
		packet.header.data_size = 0;
		c->tx_event_count++;
		syncs_client_send(c, &packet);
	}
}

void syncs_client_send_channellist(struct syncs_client *c, uint8_t sequance)
{
	struct syncs_server *s = c->server;
	struct syncs_packet packet;
	uint8_t max_channels_in_packet = SYNCS_VARIABLE_SIZE_MAXIMUM / sizeof(struct syncs_channel_info);
	struct syncs_channel_info *channel_info = (struct syncs_channel_info *) packet.buffer;
	uint8_t channel_count = 0;
	int i;
	struct syncs_channel *channel;

	packet.header.magic = SYNCS_PACKET_MAGIC;
	packet.header.magic_data = SYNCS_PACKET_MAGIC_DATA;
	packet.header.type = SYNCS_TYPE_CHANNEL_LIST;

	packet.header.id.c[0] = 0;
	packet.header.id.c[1] = (s->channel_count / max_channels_in_packet) + 1;
	packet.header.id.c[3] = sequance;
	packet.header.id.c[4] = 0;
	syncsd_debug("send channel list max channels in package %d, packages %d, seq %d", max_channels_in_packet, packet.header.id.c[1], packet.header.id.c[3]);

	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++)
		if (s->channels [i].id.i[0] != -1) {
			channel = &s->channels[i];
			channel_info[channel_count].id = channel->id;
			channel_info[channel_count].anons_count = channel->anons_count;
			channel_info[channel_count].request_count = channel->request_count;
			channel_info[channel_count].ip = channel->ticket.ip;
			channel_info[channel_count].port = channel->ticket.port;

			channel_count++;
			if (channel_count >= max_channels_in_packet) {
				syncsd_debug("send events packet with %d events", channel_count);
				packet.header.id.c[2] = channel_count;
				packet.header.data_size = channel_count * sizeof(struct syncs_channel_info);
				syncs_client_send(c, &packet);
				c->tx_event_count++;
				channel_count = 0;
				packet.header.id.c[0]++;
			}
		}
	packet.header.id.c[4] = 1;
	if (channel_count) {
		packet.header.id.c[2] = channel_count;
		packet.header.data_size = channel_count * sizeof(struct syncs_channel_info);
		syncs_client_send(c, &packet);
		c->tx_event_count++;
	} else {
		packet.header.id.c[2] = 0;
		packet.header.data_size = 0;
		c->tx_event_count++;
		syncs_client_send(c, &packet);
	}
}

int syncs_add_channel(struct syncs_client *c, syncsid_t *id, int flags, struct syncs_channel_ticket * ticket)
{
	struct syncs_server *s = c->server;
	struct syncs_channel *channel = NULL;

	syncsd_debug("looking for exist channel %s", (char *) id);
	channel = syncs_find_channel(s, id);

	if (channel == NULL) {
		channel = syncs_create_channel(s, id);
		if (channel == NULL) return -1;
	};
	channel->anons_count++;

	memcpy(&channel->ticket, ticket, sizeof(struct syncs_channel_ticket));
	channel->ticket.ip = c->addr.sin_addr.s_addr;

	if (c != channel->producer) {
		channel->producer = c;
		channel->producers_count++;
	}
	return 0;
}

int syncs_client_channel_request(struct syncs_client *c, syncsid_t *id, uint32_t flags)
{
	struct syncs_channel *channel;
	struct syncs_packet packet;

	syncsd_debug("client request a channel");
	channel = syncs_find_channel(c->server, id);
	if (channel == NULL)
		return -1;
	syncsd_debug("found channel: type [0x%08x] ", flags & SYNCS_CHANNEL_MASK);

	syncs_fill_header(&packet.header, id, SYNCS_CHANNEL_TICKET);
	packet.header.data_size = sizeof(struct syncs_channel_ticket);
	memcpy(packet.buffer, &channel->ticket, sizeof(struct syncs_channel_ticket));

	syncs_client_send(c, &packet);
	return 0;
}

int syncs_client_channel(struct syncs_client *c, struct syncs_header *packet_header, char *data)
{
	switch (packet_header->type & SYNCS_CHANNEL_MASK) {

	case SYNCS_CHANNEL_ANONS:
		syncs_add_channel(c, &packet_header->id, packet_header->type, (struct syncs_channel_ticket *) data);
		break;
	case SYNCS_CHANNEL_REQUEST:
		syncs_client_channel_request(c, &packet_header->id, packet_header->type);
		break;
	default: return -1;
	}

	return 0;
}

int syncs_client_process_packet(struct syncs_client *c, struct syncs_packet *packet)
{
	struct syncs_header *packet_header;
	char *data;

	//TODO: add packet decoder
	packet_header = &packet->header;
	data = packet->buffer;

	syncsd_debug("receive %u", packet_header->type);
	switch (packet_header->type & SYNCS_TYPE_MSG_MASK) {
	case SYNCS_TYPE_SUBSCRIBE:
		syncs_client_subscribe(c, &packet_header->id, packet_header->type, packet_header->update_counter);
		break;
	case SYNCS_TYPE_UNSUBSCRIBE:
		syncs_client_unsubscribe(c, &packet_header->id);
		break;
	case SYNCS_TYPE_DEFINE:
		syncs_add_event(c->server, &packet_header->id, packet_header->type, NULL, packet_header->data_size);
		break;
	case SYNCS_TYPE_UNDEFINE:
		syncs_free_event(c->server, &packet_header->id);
		break;
	case SYNCS_TYPE_WRITE:
		syncs_client_write(c, &packet_header->id, packet_header->type, data, packet_header->data_size);
		break;
	case SYNCS_TYPE_READ:
		syncs_client_read(c, &packet_header->id);
		break;
	case SYNCS_TYPE_CLIENT_LIST:
		syncs_client_send_clientlist(c, (int) packet_header->id.c[0]);
		break;
	case SYNCS_TYPE_EVENT_LIST:
		syncs_client_send_eventlist(c, (int) packet_header->id.c[0]);
		break;
	case SYNCS_TYPE_CLIENT_ID:
		if (packet_header->sync.data0 != SYNCS_VERSION_MAJOR) {
			syncs_send_server_status(c, SYNCS_ERROR_NOTSUPPORT);
			syncs_close_client_socket(c);
		} else
			syncs_send_server_status(c, SYNCS_ERROR_NOTFOUND);
		memcpy(&c->id, &packet_header->id, sizeof(syncsid_t));
		c->version = ((packet_header->sync.data0 & 0xff) << 8) | (packet_header->sync.data1 & 0xff);
		break;
	case SYNCS_TYPE_CHANNEL:
		syncs_client_channel(c, packet_header, data);
		break;
	case SYNCS_TYPE_CHANNEL_LIST:
		syncs_client_send_channellist(c, (int) packet_header->id.c[0]);
		break;
	default: return -1;
	}
	c->rx_event_count++;
	return 0;
}

int syncs_client_handler(void *client, uint32_t epoll_event)
{
	int read_size;
	struct syncs_client *c = client;
	int socketfd = c->socketfd;
	struct syncs_header *packet_header;
	int buffer_head = 0;
	int buffer_recv = c->buffer_recv;
	uint8_t *buffer = c->buffer;

	read_size = recv(socketfd, buffer + buffer_recv, SYNCS_CLIENT_BUFFER_SIZE - buffer_recv, 0);
	syncsd_debug("read from client %d %d bytes", c->socketfd, read_size);
	if (read_size < (int) sizeof(struct syncs_header)) {
		if (read_size == -1) {
			if ((errno == EAGAIN) || (errno == EINTR))
				return 0;
			syncs_close_client_socket(c);
		}
		if (read_size == 0) {
			syncs_close_client_socket(c);
		}
		return 0;
	}

	buffer_recv += read_size;

	while ((buffer_recv - buffer_head) >= sizeof(struct syncs_header)) {
		packet_header = (struct syncs_header *) (buffer + buffer_head);

		if (packet_header->magic != SYNCS_PACKET_MAGIC) {
			buffer_head++;
			continue;
		}
		if (packet_header->magic_data != SYNCS_PACKET_MAGIC_DATA) {
			buffer_head++;
			continue;
		}
		packet_header->data_size &= SYNCS_VARIABLE_SIZE_MAXIMUM;
		if ((buffer_recv - buffer_head) < (sizeof(struct syncs_header) +packet_header->data_size)) break;

		syncs_client_process_packet(c, (struct syncs_packet *)packet_header);
		buffer_head += sizeof(struct syncs_header) +packet_header->data_size;
	}

	if (buffer_head < buffer_recv) {
		if (buffer_head)
			memmove(buffer, buffer + buffer_head, buffer_recv - buffer_head);
		c->buffer_recv = buffer_recv - buffer_head;
	} else c->buffer_recv = 0;

	return 0;
}

struct syncs_client * syncs_add_uclient(struct syncs_server *s, struct sockaddr_in * addr)
{
	struct syncs_client *c;

	syncsd_debug("new client extended");
	c = syncs_get_free_client(s);
	if (c == NULL) {
		syncsd_error("couldn't find slot for client");
		return NULL;
	}
	c->addr_size = sizeof(struct sockaddr_in);
	memcpy(&c->addr, addr, c->addr_size);
	c->server = s;
	c->socketfd = UDP_SOCKET_STUB;

	c->event_subscribe = 0;
	c->rx_event_count = 0;
	c->tx_event_count = 0;
	c->event_write = 0;

	s->uclient_count++;
	s->client_count++;

	syncsd_debug("udp client connected %d", c->socketfd);
	return c;
}

int syncs_uclient_process_packet(struct syncs_server *s, struct sockaddr_in *addr, struct syncs_header *packet_header, char *data)
{
	struct syncs_client *c = NULL;

	if ((c = syncs_find_uclient_addr(s, addr)) == NULL) {
		if ((packet_header->type & SYNCS_TYPE_MSG_MASK) == SYNCS_TYPE_CLIENT_ID) {
			if ((c = syncs_find_uclient_id(s, &packet_header->id)) == NULL) {
				if ((c = syncs_add_uclient(s, addr)) == NULL) {
					return -1;
				}
			} else
				memcpy(&c->addr, addr, sizeof(struct sockaddr_in));
		} else {
			syncs_send_udp_server_status(s, addr, 2);
			return -1;
		}
	}
	syncs_client_process_packet(c, (struct syncs_packet *)packet_header);
	return 0;
}

int syncs_udp_handler(void *server, uint32_t epoll_event)
{
	struct syncs_server *s = server;
	struct syncs_header *packet_header;
	uint8_t *buffer = s->ubuffer;
	int read_size;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int ret;

	read_size = recvfrom(s->usocketfd, buffer, SYNCS_CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &addr, &addr_len);
	syncsd_debug("read from %i udp %d bytes", s->usocketfd, read_size);
	if (read_size < (int) sizeof(struct syncs_header)) {
		syncsd_error("read from udp %d bytes errno %i", read_size, errno);
		if (read_size == -1) {
			if ((errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT || errno == EHOSTDOWN ||
				errno == ENONET || errno == EHOSTUNREACH || errno == EOPNOTSUPP || errno == ENETUNREACH)) {
				return 0;
			};
		}
		if (read_size == 0) {
		}
		return 0;
	}

	packet_header = (struct syncs_header *) (buffer);
	if (packet_header->magic != SYNCS_PACKET_MAGIC) {
		return 0;
	}
	if (packet_header->magic_data != SYNCS_PACKET_MAGIC_DATA) {
		return 0;
	}
	packet_header->data_size &= SYNCS_VARIABLE_SIZE_MAXIMUM;
	ret = syncs_uclient_process_packet(s, &addr, packet_header, (char *) packet_header + sizeof(struct syncs_header));
	syncsd_debug("syncs_uclient_process_packet return %i", ret);
	return ret;
}

int syncs_add_client(void *server, uint32_t epoll_event)
{
	struct syncs_server *s = server;
	struct syncs_client *c;
	struct epoll_event socket_event;

	syncsd_debug("new client extended");
	c = syncs_get_free_client(s);
	if (c == NULL) {
		syncsd_error("couldn't find slot for client");
		return 0;
	}

	c->addr_size = sizeof(struct sockaddr_in);
	c->server = s;

	c->socketfd = accept(s->socketfd, (struct sockaddr *) &c->addr, (socklen_t*) & c->addr_size);
	if (c->socketfd == -1) {
		if ((errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT || errno == EHOSTDOWN ||
			errno == ENONET || errno == EHOSTUNREACH || errno == EOPNOTSUPP || errno == ENETUNREACH)) {
			return 0;
		};
		syncsd_error("error on wait client: %s", strerror(errno));
		return -1;
	}
	syncs_set_nonblocking_socket(c->socketfd, 1024 * 1024, 1024 * 1024);
	syncs_set_keepalive(c->socketfd, 600, 3);

	c->epoll_data.socket = c;
	c->epoll_data.cb = &syncs_client_handler;
	c->event_subscribe = 0;
	c->rx_event_count = 0;
	c->tx_event_count = 0;
	c->event_write = 0;

	socket_event.data.ptr = &c->epoll_data;
	socket_event.events = EPOLLIN | EPOLLERR;
	epoll_ctl(s->epollfd, EPOLL_CTL_ADD, c->socketfd, &socket_event);
	s->client_count++;

	syncsd_debug("client connected %d", c->socketfd);
	return 0;
}

void syncs_recv_clients(struct syncs_server * s)
{
	int epollfd = s->epollfd;
	struct epoll_event socket_event;
	struct epoll_event *socket_events = s->socket_events;
	int event_size;
	struct syncs_epoll_cb *epoll_data;
	int i;

	socket_event.data.ptr = &s->epoll_data;
	socket_event.events = EPOLLIN | EPOLLERR;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, s->socketfd, &socket_event);
	socket_event.data.ptr = &s->epoll_udpdata;
	socket_event.events = EPOLLIN | EPOLLERR;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, s->usocketfd, &socket_event);

	while (1) {
		event_size = epoll_wait(epollfd, socket_events, SYNCS_CLIENT_MAXIMUM, -1);
		for (i = 0; i < event_size; i++) {
			syncsd_debug("event %d from %d", i, event_size);
			epoll_data = (struct syncs_epoll_cb *) socket_events[i].data.ptr;
			epoll_data->cb(epoll_data->socket, socket_events[i].events);
		}
	}
}

void *syncs_server_thread(void *server)
{
	struct syncs_server *s = server;

	syncsd_debug("run server thread");
	while (1) {
		syncsd_debug("open server socket");
		s->socketfd = syncs_tcpserver_open(s->addr, s->port);
		if (s->socketfd < 0) {
			syncsd_error("couldn't open tcp socket");
			goto error_tcp;
		}
		syncs_set_nonblocking_socket(s->socketfd, 1024 * 1024, 1024 * 1024);

		s->usocketfd = syncs_udpserver_open(s->addr, s->port);
		if (s->usocketfd < 0) {
			syncsd_error("couldn't open udp socket");
			goto error_udp;
		}
		syncs_set_nonblocking_socket(s->usocketfd, 1024 * 1024, 1024 * 1024);

		s->epollfd = epoll_create(SYNCS_CLIENT_MAXIMUM + 2); // actually arg is ignore
		if (s->epollfd < 0) {
			syncsd_error("couldn't create epoll descriptor");
			goto error_epoll;
		}
		syncs_recv_clients(s);
		close(s->epollfd);
error_epoll:
		close(s->socketfd);
error_udp:
		close(s->usocketfd);
error_tcp:
		usleep(300000);
	}

	free(server);
	return NULL;
}

void syncs_server_set_sync_offset(struct syncs_server *s, int ms)
{
	s->sync_offset = ms;
}

static void syncs_server_structure_init(struct syncs_server * s)
{
	int i;

	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++) {
		s->events[i].id.i[0] = -1;
	}
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++) {
		s->channels[i].id.i[0] = -1;
	}
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++) {
		s->clients[i].socketfd = -1;
	}
	s->sync_offset = SYNCS_DEFAULT_SYNC_OFFSET_MS;
}

struct syncs_server * syncs_server_create(const char *addr, int port, const char *cid)
{
	struct syncs_server *s;

	s = calloc(1, sizeof(struct syncs_server));
	if (s == NULL) {
		syncsd_error("couldn't allocate a few memory");
		goto error_server_alloc;
	}

	syncs_server_structure_init(s);
	if (addr != NULL)
		strncpy(s->addr, addr, 20);
	s->port = port;
	if (cid != NULL)
		syncs_idstr(&s->id, cid);
	s->epoll_data.socket = s;
	s->epoll_data.cb = &syncs_add_client;
	s->epoll_udpdata.socket = s;
	s->epoll_udpdata.cb = &syncs_udp_handler;

	pthread_create(&s->thread, NULL, &syncs_server_thread, (void*) s);
	return s;

error_server_alloc:
	return NULL;
}

void syncs_server_stop(struct syncs_server * s)
{
	/* SyncScribe will live forever */
}

void syncs_server_print_event(struct syncs_server * s, FILE *stream)
{
	int i;
	char str[15];
	struct in_addr addr;

	(void) addr;
	fprintf(stream, "Event statistics\n");
	fprintf(stream, "|%30s|%15s|%7s|%7s|%7s", "id", "value", "count", "prod.", "cons.\n");
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (s->events[i].id.i[0] != -1) {
			switch (s->events[i].data_type) {
			case SYNCS_TYPE_VAR_INT32:
				if ((*(int *) s->events[i].data < 255) && isprint(*(int *) s->events[i].data)) snprintf(str, 15, "%d/%c", *(int *) s->events[i].data, *(char *) s->events[i].data);
				else snprintf(str, 15, "%d", *(int *) s->events[i].data);
				break;
			case SYNCS_TYPE_VAR_INT64: snprintf(str, 15, "%ld", *(long *) s->events[i].data);
				break;
			case SYNCS_TYPE_VAR_FLOAT: snprintf(str, 15, "%f", *(float *) s->events[i].data);
				break;
			case SYNCS_TYPE_VAR_DOUBLE: snprintf(str, 15, "%lf", *(double *) s->events[i].data);
				break;
			case SYNCS_TYPE_VAR_STRING: snprintf(str, 15, "%s", (char *) s->events[i].data);
				break;
			default: snprintf(str, 10, "not support");
				break;
			}
			fprintf(stream, "|%30s|%15s|%7d|%7d|%7d\n", (char *) &s->events[i].id, str, s->events[i].count, s->events[i].producers_count, s->events[i].consumers_count);
		}

	fprintf(stream, "Client statistics\n");
	fprintf(stream, "|%20s|%7s|%7s|%7s|%7s|%7s|%7s\n", "id", "rx pkt", "tx pkt", "subscr", "write", "ip", "proto");
	for (i = 0; i < SYNCS_CLIENT_MAXIMUM; i++)
		if (s->clients[i].socketfd != -1) {
			fprintf(stream, "|%20s|%7d|%7d|%7d|%7d|%7s|%7s\n", (char *) &s->clients[i].id, s->clients[i].rx_event_count, s->clients[i].tx_event_count, s->clients[i].event_subscribe, s->clients[i].event_write,
				inet_ntoa(s->clients[i].addr.sin_addr), (s->clients[i].socketfd == UDP_SOCKET_STUB) ? "udp" : "tcp");
		}

	fprintf(stream, "Channel statistics\n");
	fprintf(stream, "|%20s|%11s|%7s|%7s|%7s|%7s", "id", "ip", "port", "tickets", "prod", "cons.\n");
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++)
		if (s->channels[i].id.i[0] != -1) {
			addr.s_addr = s->channels[i].ticket.ip;
			fprintf(stream, "|%20s|%11s|%7d|%7d|%7d|%7d\n", (char *) &s->channels[i].id, inet_ntoa(addr), s->channels[i].ticket.port,
				s->channels[i].request_count, s->channels[i].producers_count, s->channels[i].anons_count);
		}

}

int syncs_server_ssdp_response(struct syncs_server *s, char *buffer, uint32_t size)
{
	return(snprintf(buffer, size, "%sCACHE-CONTROL:max-age=120\r\nDATE:\r\nEXT:\r\nLOCATION:%s:%d\r\nSERVER:unknow\r\nST:%s\r\nUSN:%s\r\n\r\n",
		ssdp_headers.response, s->addr, s->port, syncs_ssdp_field.name, s->id.c));
}

void syncs_server_ssdp_receive(struct syncs_server * s)
{
	struct sockaddr_in gaddr;
	char ssdp_client_buffer[SSDP_PACKET_SIZE];
	struct sockaddr_in ssdp_client_addr;
	socklen_t ssdp_client_addr_len = sizeof(struct sockaddr_in);
	ssize_t packet_size;
	int ssdp_msearch_size = strlen(ssdp_headers.msearch);


	syncs_set_multicast_group(&gaddr, ssdp_network.ip, ssdp_network.port);

	if (s->ssdp_beacon)
		syncs_set_rxtimeout(s->ssdp_socketfd, 0, 500000);

	while (1) {
		syncsd_debug("wait for ssdp packet");
		packet_size = recvfrom(s->ssdp_socketfd, ssdp_client_buffer, SSDP_PACKET_SIZE, 0, (struct sockaddr *) &ssdp_client_addr, &ssdp_client_addr_len);
		if (packet_size < 0) {
			if ((errno == EAGAIN) && s->ssdp_beacon) {
				packet_size = syncs_server_ssdp_response(s, ssdp_client_buffer, SSDP_PACKET_SIZE);
				syncs_udp_send(s->ssdp_socketfd, ssdp_client_buffer, packet_size, &gaddr, sizeof(struct sockaddr_in));
			}
			return;
		}
		if (packet_size < ssdp_msearch_size) continue;

		if (memcmp(ssdp_client_buffer, ssdp_headers.msearch, ssdp_msearch_size))
			continue;
		if (strstr(ssdp_client_buffer, syncs_ssdp_field.name) == NULL)
			continue;
		packet_size = syncs_server_ssdp_response(s, ssdp_client_buffer, SSDP_PACKET_SIZE);
		syncs_udp_send (s->ssdp_socketfd, ssdp_client_buffer, packet_size, &gaddr, sizeof(struct sockaddr_in));
	}
}

void *syncs_server_ssdp_thread(void *args)
{
	struct syncs_server *s = args;

	syncsd_debug("run ssdp thread");
	while (1) {
		syncsd_debug("open server socket");
		s->ssdp_socketfd = syncs_udpmulticast_open(NULL, ssdp_network.port);
		if (syncs_add_multicast_group(s->ssdp_socketfd, ssdp_network.ip, NULL)) {
			syncs_add_multicast_route(NULL);
			sleep(1);
			continue;
		}
		if (s->ssdp_socketfd < 0) {
			syncsd_error("couldn't open ssdp socket");
			continue;
		}
		syncs_server_ssdp_receive(s);
		close(s->ssdp_socketfd);
	}

	return NULL;
}

void syncs_server_ssdp_create(struct syncs_server *s, const char *address, int beacon)
{
	s->ssdp_beacon = beacon;
	pthread_create(&s->ssdp_thread, NULL, &syncs_server_ssdp_thread, (void*) s);
}

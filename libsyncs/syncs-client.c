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
#include <pthread.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <ctype.h>

#include "syncs-net.h"
#include "syncs-common.h"
#include "syncs-crypt.h"
#include "syncs-client-types.h"

#define MODULE_NAME "syncs-client"
#include <syncs-debug.h>
// #undef syncsd_debug
// #define syncsd_debug(fmt,args...)

extern int syncs_find_server(char *addr, int *port);

static int syncs_connect_send(struct syncs_connect *c, void *buffer, uint32_t size)
{
	if (c->socketfd > 0)
		return(syncs_blocking_send(c->socketfd, buffer, size, MSG_NOSIGNAL));
	else if (c->usocketfd > 0) {
		syncs_udp_send(c->usocketfd, buffer, size, &c->saddr, c->saddr_size);
		return 0;
	}
	return -1;
}

static struct syncs_client_event *syncs_find_event(struct syncs_connect *s, syncsid_t *id)
{
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (syncs_idcmp(&s->events[i].id, id)) {
			return &s->events[i];
		}
	return NULL;
}

static struct syncs_client_event *syncs_find_event_str(struct syncs_connect *s, const char *cid)
{
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (syncs_idcmp_str(&s->events[i].id, cid)) {
			return &s->events[i];
		}
	return NULL;
}

static struct syncs_client_event *syncs_get_free_event(struct syncs_connect *s)
{
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++)
		if (s->events[i].id.i[0] == -1) {
			return &s->events[i];
		}
	return NULL;
}

static struct syncs_client_event *syncs_get_event_str(struct syncs_connect *s, const char *cid)
{
	struct syncs_client_event *event = syncs_find_event_str(s, cid);
	if (event == NULL)
		return syncs_get_free_event (s);
	return event;
}


static void syncs_wait_sync(struct syncdata *sync)
{
	struct timespec system_time, diff, sync_time;
	sync_time.tv_sec = sync->data0;
	sync_time.tv_nsec = sync->data1;
	while (1) {
		clock_gettime(CLOCK_REALTIME, &system_time);
		diff.tv_sec = sync_time.tv_sec - system_time.tv_sec;
		diff.tv_nsec = sync_time.tv_nsec - system_time.tv_nsec;
		if ((diff.tv_sec < 0) || ((diff.tv_sec == 0) && (diff.tv_nsec < 0)) || (diff.tv_sec > 1)) break;
		if (diff.tv_nsec < 0) {
			diff.tv_nsec += 1000000000L;
			diff.tv_sec--;
		}
		syncsd_debug("wait for %ld:%ld", (long int) diff.tv_sec, (long int) diff.tv_nsec);

		if ((diff.tv_sec > 0) || (diff.tv_nsec > 1000000L)) {
			nanosleep(&diff, NULL);
		}
	}
}


static int syncs_wait_for(int *wait, pthread_mutex_t *mutex, pthread_cond_t *cond, unsigned int timeout_sec)
{
	struct timespec to;

	clock_gettime(CLOCK_MONOTONIC, &to);
	to.tv_sec += timeout_sec;

	pthread_mutex_lock(mutex);
	while (*wait)
		if (pthread_cond_timedwait(cond, mutex, &to) == ETIMEDOUT) {
			pthread_mutex_unlock(mutex);
			return -ETIMEDOUT;
		}
	pthread_mutex_unlock(mutex);
	return 0;
}

static void syncs_notify_for(int *wait, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
	pthread_mutex_lock(mutex);
	*wait = 0;
	pthread_cond_signal(cond);
	pthread_mutex_unlock(mutex);
}

static int syncs_wait_for_read(struct syncs_connect *s, unsigned int timeout)
{
	return syncs_wait_for(&s->read_wait, &s->read_mutex, &s->read_cond, timeout);
}

int syncs_read(struct syncs_connect *s, uint32_t flags, const char *cid, void *data, uint32_t *data_size)
{
	struct syncs_packet packet;
	int size = sizeof(struct syncs_header);

	if (s->socketfd < 0)
		return -EBADFD;

	s->read_wait = 1;
	syncs_fill_header_request_str (&packet.header, cid, SYNCS_TYPE_READ | (flags & SYNCS_TYPE_VAR_MASK));
	syncs_idcpy(&s->read_id, &packet.header.id);
	syncs_connect_send(s, &packet, size);

	if (syncs_wait_for_read(s, 3))
		return -ETIMEDOUT;

	if (*data_size > s->read_size)
		*data_size = s->read_size;
	memcpy(data, s->read_data, *data_size);

	syncsd_debug("read [%s] type 0x%08x, data_size %d, ret size %d", cid, packet.header.type, packet.header.data_size, *data_size);
	return 0;
}

int syncs_read_int32(struct syncs_connect *s, uint32_t flags, const char *id, uint32_t *data)
{
	uint32_t size = syncs_get_size_by_type(SYNCS_TYPE_VAR_INT32);
	return(syncs_read(s, flags | SYNCS_TYPE_VAR_INT32, id, data, &size));
}

int syncs_read_int64(struct syncs_connect *s, uint32_t flags, const char *id, uint64_t *data)
{
	uint32_t size = syncs_get_size_by_type(SYNCS_TYPE_VAR_INT64);
	return(syncs_read(s, flags | SYNCS_TYPE_VAR_INT64, id, data, &size));
}

int syncs_read_float(struct syncs_connect *s, uint32_t flags, const char *id, float *data)
{
	uint32_t size = syncs_get_size_by_type(SYNCS_TYPE_VAR_FLOAT);
	return(syncs_read(s, flags | SYNCS_TYPE_VAR_FLOAT, id, data, &size));
}

int syncs_read_double(struct syncs_connect *s, uint32_t flags, const char *id, double *data)
{
	uint32_t size = syncs_get_size_by_type(SYNCS_TYPE_VAR_DOUBLE);
	return(syncs_read(s, flags | SYNCS_TYPE_VAR_DOUBLE, id, data, &size));
}

int syncs_read_str(struct syncs_connect *s, uint32_t flags, const char *id, char *data, uint32_t size)
{
	return(syncs_read(s, flags | SYNCS_TYPE_VAR_STRING, id, data, &size));
}

int syncs_write(struct syncs_connect *s, uint32_t flags, const char *cid, void *data, uint32_t data_size)
{
	struct syncs_packet packet;
	uint32_t size = sizeof(struct syncs_header);
	int ret;

	syncsd_debug("data ptr %p", s);
	syncs_idstr(&packet.header.id, cid);

	syncs_fill_header_str (&packet.header, cid, SYNCS_TYPE_WRITE | (flags & (SYNCS_TYPE_VAR_MASK | SYNCS_TYPE_FLAGS_MASK)));

	if (!data_size)
		data_size = syncs_get_size_by_type(flags);
	if (data_size && data != NULL) {
		memcpy(packet.buffer, data, data_size);
		packet.header.data_size = data_size & SYNCS_VARIABLE_SIZE_MAXIMUM;
		size += packet.header.data_size;
	} else packet.header.data_size = 0;

	ret = syncs_connect_send(s, &packet, size);
	syncsd_debug("write [%s] type 0x%08x, data_size %d", cid, packet.header.type, packet.header.data_size);
	return ret;
}

int syncs_write_int32(struct syncs_connect *s, uint32_t flags, const char *id, int32_t data)
{
	syncsd_debug("data ptr %p", s);
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_INT32, id, &data, 0));
}

int syncs_write_int64(struct syncs_connect *s, uint32_t flags, const char *id, int64_t data)
{
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_INT64, id, &data, 0));
}

int syncs_write_float(struct syncs_connect *s, uint32_t flags, const char *id, float data)
{
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_FLOAT, id, &data, 0));
}

int syncs_write_double(struct syncs_connect *s, uint32_t flags, const char *id, double data)
{
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_DOUBLE, id, &data, 0));
}

int syncs_write_str(struct syncs_connect *s, uint32_t flags, const char *id, const char *data)
{
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_STRING, id, (void *) data, strlen(data) + 1));
}

int syncs_write_event(struct syncs_connect *s, uint32_t flags, const char *id)
{
	return(syncs_write(s, flags | SYNCS_TYPE_VAR_EMPTY, id, NULL, 0));
}

const char *syncs_wait_event(struct syncs_connect *s, uint32_t *flags, void *data, uint32_t *data_size, unsigned int timeout_sec)
{
	struct syncs_client_event *event = s->events_queue;

	if (event == NULL) {
		if (syncs_wait_for(&s->event_wait, &s->event_wait_mutex, &s->event_wait_cond, timeout_sec))
			return NULL;
		event = s->events_queue;
		if (event == NULL)
			return NULL;
	}

	pthread_mutex_lock(&event->data_mutex);
	*flags = event->flags;
	if (*data_size > event->data_size)
		*data_size = event->data_size;
	memcpy(data, event->data, *data_size);
	s->events_queue = NULL;
	s->event_wait = 1;
	pthread_mutex_unlock(&event->data_mutex);

	return event->id.c;
}

int syncs_client_send_channel_anons(struct syncs_connect *s, syncsid_t *id, struct syncs_channel_ticket *ticket)
{
	struct syncs_packet packet;
	uint32_t size = sizeof(struct syncs_header) + sizeof(struct syncs_channel_ticket);

	syncs_fill_header(&packet.header, id, SYNCS_TYPE_CHANNEL | SYNCS_CHANNEL_ANONS);
	packet.header.data_size = sizeof(struct syncs_channel_ticket);

	memcpy(packet.buffer, ticket, sizeof(struct syncs_channel_ticket));
	syncs_connect_send(s, &packet, size);
	return 0;
}

int syncs_define(struct syncs_connect *s, const char *cid, uint32_t flags)
{
	struct syncs_packet packet;
	uint32_t size = sizeof(struct syncs_header);

	syncs_fill_header_request_str(&packet.header, cid, SYNCS_TYPE_DEFINE | (flags & (SYNCS_TYPE_VAR_MASK | SYNCS_TYPE_FLAGS_MASK)));
	syncs_connect_send(s, &packet, size);
	syncsd_debug("sent define event");
	return 0;
}

int syncs_undefine(struct syncs_connect *s, const char *cid)
{
	struct syncs_packet packet;
	uint32_t size = sizeof(struct syncs_header);

	syncs_fill_header_request_str(&packet.header, cid, SYNCS_TYPE_DEFINE);
	syncs_connect_send(s, &packet, size);
	syncsd_debug("sent undefine event");
	return 0;
}

static int syncs_client_send_subscribe(struct syncs_connect *s, syncsid_t *id, int flags, uint64_t update_counter)
{
	struct syncs_packet packet;
	int size = sizeof(struct syncs_header);

	syncs_fill_header_request_id(&packet.header, id, SYNCS_TYPE_SUBSCRIBE | (flags & (SYNCS_TYPE_VAR_MASK | SYNCS_TYPE_FLAGS_MASK)));
	packet.header.update_counter = update_counter;

	syncs_connect_send(s, &packet, size);
	syncsd_debug("sent subscribe event");
	return 0;
}

static int syncs_client_send_unsubscribe(struct syncs_connect *s, syncsid_t *id, int flags)
{
	struct syncs_packet packet;
	int size = sizeof(struct syncs_header);

	syncs_fill_header_request_id(&packet.header, id, SYNCS_TYPE_UNSUBSCRIBE | (flags & (SYNCS_TYPE_VAR_MASK | SYNCS_TYPE_FLAGS_MASK)));
	syncs_connect_send(s, &packet, size);
	syncsd_debug("sent unsubscribe event");
	return 0;
}

static int syncs_client_send_id(struct syncs_connect *s, syncsid_t *id)
{
	struct syncs_header packet;

	syncs_fill_header_request_id(&packet, id, SYNCS_TYPE_CLIENT_ID);
	packet.sync.data0 = SYNCS_VERSION_MAJOR;
	packet.sync.data1 = SYNCS_VERSION_MINOR;

	syncs_connect_send(s, &packet, sizeof(struct syncs_header));
	syncsd_debug("sent client id");
	return 0;
}

static void syncs_send_id(struct syncs_connect * s)
{
	int i;
	syncs_client_send_id(s, &s->id);
	syncsd_debug("send subscribe events");
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++) {
		if (s->events[i].id.i[0] != -1)
			syncs_client_send_subscribe(s, &s->events[i].id, s->events[i].flags, s->events[i].update_counter);
	}
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++) {
		if (s->channels[i].id.i[0] != -1)
			syncs_client_send_channel_anons(s, &s->channels[i].id, &s->channels[i].ticket);
	}
}

int syncs_subscribe_event(struct syncs_connect *s, int flags, const char *cid, void (*cb)(void *, char *, void *, uint32_t), void *args)
{
	struct syncs_client_event *event = syncs_get_event_str(s, cid);
	if (event == NULL)
		return -ENOMEM;

	syncsd_debug("data ptr s = %p; id = %s cb = %p event = %p ", s, cid, cb, event);

	event->cb = cb;
	event->args = args;
	event->flags = flags;
	syncs_idstr(&event->id, cid);
	if (s->socketfd >= 0) {
		syncs_client_send_subscribe(s, &event->id, flags, event->update_counter);
		syncsd_debug("event registrated");
	}
	return 0;
}

int syncs_subscribe_event_sync_user(struct syncs_connect *s, uint32_t flags, const char *cid, void *user_data, uint32_t user_data_size)
{
	struct syncs_client_event *event = syncs_get_event_str(s, cid);
	if (event == NULL)
		return -ENOMEM;

	syncsd_debug("data ptr s = %p; id = %s data = %p event = %p ", s, cid, user_data, event);

	pthread_mutex_lock(&event->data_mutex);
	if (event->data != NULL)
		free(event->data);
	event->data = user_data;
	event->data_user_size = user_data_size;
	pthread_mutex_unlock(&event->data_mutex);

	event->flags = flags;
	syncs_idstr(&event->id, cid);
	if (s->socketfd >= 0) {
		syncs_client_send_subscribe(s, &event->id, flags, event->update_counter);
		syncsd_debug("sync event registrated");
	}
	return 0;
}

int syncs_subscribe_event_sync(struct syncs_connect *s, uint32_t flags, const char *cid)
{
	uint8_t *event_data;
	struct syncs_client_event *event = syncs_get_event_str(s, cid);
	if (event == NULL)
		return -ENOMEM;

	event_data = malloc (SYNCS_EVENT_DATA_SIZE_MAXIMUM);
	if (event == NULL)
		return -ENOMEM;

	pthread_mutex_lock(&event->data_mutex);
	if (event->data != NULL)
		free(event->data);
	event->data = event_data;
	event->data_user_size = SYNCS_EVENT_DATA_SIZE_MAXIMUM;
	pthread_mutex_unlock(&event->data_mutex);

	event->flags = flags;
	syncs_idstr(&event->id, cid);
	if (s->socketfd >= 0) {
		syncs_client_send_subscribe(s, &event->id, flags, event->update_counter);
		syncsd_debug("sync event registrated");
	}
	return 0;
}

void syncs_unsubscribe_event(struct syncs_connect *s, const char *cid)
{
	struct syncs_client_event *event;
	syncsid_t id;

	syncs_idstr(&id, cid);
	event = syncs_find_event(s, &id);
	if (event != NULL) {
		syncs_client_send_unsubscribe(s, &event->id, event->flags);
		event->id.i[0] = -1;
	}
	return;
}

void syncs_process_event(struct syncs_connect *s, struct syncs_packet *packet)
{
	struct syncs_client_event *event;
	void (*cb)(void *, char *, void *, uint32_t);
	syncsid_t *id = &(packet->header.id);
	void *data = packet->buffer;
	uint16_t data_size = packet->header.data_size;
	uint64_t update_counter = packet->header.update_counter;

	event = syncs_find_event(s, id);
	if (event != NULL) {
		cb = event->cb;
		syncsd_debug("cb = %p", cb);
		if (cb != NULL)
			cb(event->args, (char *) &event->id, data, data_size);
		if (event->data != NULL) {
			pthread_mutex_lock(&event->data_mutex);
			memcpy(event->data, data, data_size);
			event->data_size = data_size;
			s->events_queue = event;
			pthread_mutex_unlock(&event->data_mutex);
			syncs_notify_for(&s->event_wait, &s->event_wait_mutex, &s->event_wait_cond);
		}
		event->update_counter = update_counter;
	}
}

static void syncs_channel_process_packet(struct syncs_connect * s, struct syncs_packet *packet)
{
	switch (packet->header.type & SYNCS_TYPE_CHANNEL_MASK) {
	case SYNCS_CHANNEL_TICKET:
		syncsd_debug("receive ticket id %s type 0x%08x", packet->header.id.c, packet->header.type);
		pthread_mutex_lock(&s->ticket_mutex);
		if (s->ticket_wait && syncs_idcmp(&packet->header.id, &s->ticket_id)) {
			memcpy(&s->ticket_data, packet->buffer, sizeof(struct syncs_channel_ticket));
			s->ticket_wait = 0;
			pthread_cond_signal(&s->ticket_cond);
		}
		pthread_mutex_unlock(&s->ticket_mutex);
		break;
	}
}

static void syncs_process_error(struct syncs_connect *s, struct syncs_packet *packet)
{
	switch (packet->header.update_counter) {
	case SYNCS_ERROR_NOTSUPPORT:
		s->onexit = 1;
		syncsd_error("Server does not support the client protocol version");
		break;
	case SYNCS_ERROR_CRYPT:
		s->onexit = 1;
		syncsd_error("The security token doesn't match with server");
		break;
	case SYNCS_ERROR_UNKNOWNCLIENT:
		syncs_send_id(s);
		break;
	}
}

static void syncs_process_packet(struct syncs_connect * s, struct syncs_packet *packet)
{
	struct syncs_header *packet_header;
	char *data;

	//TODO: add packet decoder
	packet_header = &packet->header;
	data = packet->buffer;

	switch (packet_header->type & SYNCS_TYPE_MSG_MASK) {
	case SYNCS_TYPE_EVENT:
		syncsd_debug("receive event id %s type 0x%08x", packet_header->id.c, packet_header->type);
		if (packet_header->type & SYNCS_TYPE_SYNC)
			syncs_wait_sync(&(packet_header->sync));
		syncs_process_event(s, packet);
		break;
	case SYNCS_TYPE_CHANNEL:
		syncs_channel_process_packet(s, packet);
		break;
	case SYNCS_TYPE_SERVER_STATUS:
		syncs_idcpy(&s->server_id, &packet_header->id);
		syncs_process_error(s, packet);
		syncsd_debug("receive server status id %s type 0x%08x", s->server_id.c, packet_header->type);
		break;
	case SYNCS_TYPE_READ:
		syncsd_debug("receive read id %s type 0x%08x", packet_header->id.c, packet_header->type);
		pthread_mutex_lock(&s->read_mutex);
		if (s->read_wait && syncs_idcmp(&packet_header->id, &s->read_id)) {
			s->read_size = packet_header->data_size;
			memcpy(s->read_data, packet->buffer, packet_header->data_size);
			s->read_wait = 0;
			pthread_cond_signal(&s->read_cond);
		}
		pthread_mutex_unlock(&s->read_mutex);
		break;
	case SYNCS_TYPE_EVENT_LIST:
		syncsd_debug("receive event list seq %d[%d] packet %d[%d] end %d count %d", packet_header->id.c[3], s->eventlist_sequence,
			packet_header->id.c[0], s->eventlist_sequence,
			packet_header->id.c[4], packet_header->id.c[2]);
		if (s->events_info == NULL) return;
		if (packet_header->id.c[3] != s->eventlist_sequence) return;
		if (packet_header->id.c[0] != s->eventlist_wait_packet) return;

		syncsd_debug("coping %d event list", packet_header->data_size);
		memcpy(&s->events_info[s->eventlist_recv], data, packet_header->data_size);
		s->eventlist_recv += packet_header->id.c[2];
		s->eventlist_wait_packet++;
		if (packet_header->id.c[4] == 1) {
			syncsd_debug("releasing waiter");
			syncs_notify_for(&s->eventlist_wait, &s->eventlist_mutex, &s->eventlist_cond);
		}
		break;
	case SYNCS_TYPE_CLIENT_LIST:
		syncsd_debug("receive client list seq %d packet %d end %d count %d", packet_header->id.c[3], packet_header->id.c[0], packet_header->id.c[4], packet_header->id.c[2]);
		if (s->clients_info == NULL) return;
		if (packet_header->id.c[3] != s->clientlist_sequence) return;
		if (packet_header->id.c[0] != s->clientlist_wait_packet) return;

		syncsd_debug("coping client list");
		memcpy(&s->clients_info[s->clientlist_recv], data, packet_header->data_size);
		s->clientlist_recv += packet_header->id.c[2];
		s->clientlist_wait_packet++;
		if (packet_header->id.c[4] == 1) {
			syncsd_debug("releasing waiter");
			syncs_notify_for(&s->clientlist_wait, &s->clientlist_mutex, &s->clientlist_cond);
		}
		break;
	case SYNCS_TYPE_CHANNEL_LIST:
		syncsd_debug("receive channel list seq %d[%d] packet %d[%d] end %d count %d", packet_header->id.c[3], s->channellist_sequence,
			packet_header->id.c[0], s->channellist_sequence,
			packet_header->id.c[4], packet_header->id.c[2]);
		if (s->channels_info == NULL) return;
		if (packet_header->id.c[3] != s->channellist_sequence) return;
		if (packet_header->id.c[0] != s->channellist_wait_packet) return;

		syncsd_debug("coping %d channel list", packet_header->data_size);
		memcpy(&s->channels_info[s->channellist_recv], data, packet_header->data_size);
		s->channellist_recv += packet_header->id.c[2];
		s->channellist_wait_packet++;
		if (packet_header->id.c[4] == 1) {
			syncsd_debug("releasing waiter");
			syncs_notify_for(&s->channellist_wait, &s->channellist_mutex, &s->channellist_cond);
		}

		break;
	}
}

void *syncs_connect_cb_thread(void *server)
{
	struct syncs_connect *s = server;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	s->connect_cb(s->connect_arg);
	s->connect_cb_status = 0;
	pthread_exit(0);
}

static void syncs_recv(struct syncs_connect * s)
{
	int socketfd = s->socketfd;
	fd_set set;
	int res;

	int read_size;
	struct syncs_header *packet_header;
	int buffer_head = 0;
	int buffer_recv = 0;
	uint8_t *buffer = s->buffer;

	syncs_send_id(s);
	if ((s->connect_cb != NULL) && (s->connect_cb_status == 0)) {
		s->connect_cb_status = 1;
		pthread_create(&s->connect_thread, NULL, &syncs_connect_cb_thread, (void*) s);
	}

	syncs_notify_for(&s->connect_wait, &s->connect_mutex, &s->connect_cond);

	FD_ZERO(&set);
	FD_SET(socketfd, &set);
	syncsd_debug("start receive data");
	s->ready = 1;
	while (!s->onexit) {
		res = select(socketfd + 1, &set, NULL, NULL, NULL);
		syncsd_debug("select return %d, errno %d, error[%s]", res, errno, strerror(errno));
		if (res == 0) continue;
		if ((res == -1) && (errno == EINTR)) {
			continue;
		}
		read_size = recv(socketfd, buffer + buffer_recv, SYNCS_CLIENT_BUFFER_SIZE - buffer_recv, 0);
		syncsd_debug("recv return %d, errno %d, error[%s]", read_size, errno, strerror(errno));
		if (read_size < (int) sizeof(struct syncs_header)) {
			if (read_size == -1) {
				if ((errno == EAGAIN) || (errno == EINTR))
					continue;
				break;
			}
			if (read_size == 0) {
				break;
			}
			continue;
		}

		buffer_recv += read_size;
		buffer_head = 0;

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
			if ((buffer_recv - buffer_head) < (sizeof(struct syncs_header) + packet_header->data_size)) break;
			syncs_process_packet(s, (struct syncs_packet *)packet_header);
			buffer_head += sizeof(struct syncs_header) +packet_header->data_size;
		}

		if (buffer_head < buffer_recv) {
			if (buffer_head)
				memmove(buffer, buffer + buffer_head, buffer_recv - buffer_head);
			buffer_recv = buffer_recv - buffer_head;
		} else buffer_recv = 0;
	}
	s->ready = 0;
}

void *syncs_connect_thread(void *server)
{
	struct syncs_connect *s = server;

	while ((s->addr[0] == 0) || (s->port == 0)) {
		if (!syncs_find_server(s->addr, &s->port))
			break;
		syncsd_info("Still looking for tcp server");
	}

	syncsd_debug("start client thread");
	while (!s->onexit) {
		s->socketfd = syncs_tcpclient_open(s->addr, s->port);
		if (s->socketfd < 0) {
			// syncsd_error("could't open socket");
			usleep(300000);
			continue;
		}
		syncs_set_nonblocking_socket(s->socketfd, 1024 * 1024, 1024 * 1024);
		syncs_set_keepalive(s->socketfd, 30, 3);
		fcntl(s->socketfd, F_SETFD, FD_CLOEXEC);
		syncs_recv(s);
		shutdown(s->socketfd, SHUT_RDWR);
		s->socketfd = -1;
		usleep(1000000);
	}
	return NULL;
}

static void syncs_udprecv(struct syncs_connect * s)
{
	int usocketfd = s->usocketfd;
	fd_set set;
	int res;

	int read_size;
	struct syncs_header *packet_header;
	uint8_t *buffer = s->buffer;
	struct sockaddr_in addr;
	socklen_t addr_len;

	syncs_send_id(s);

	pthread_mutex_lock(&s->connect_mutex);
	s->connect_wait = 0;
	pthread_cond_signal(&s->connect_cond);
	pthread_mutex_unlock(&s->connect_mutex);

	FD_ZERO(&set);
	FD_SET(usocketfd, &set);
	syncsd_debug("start receive data");

	s->ready = 1;
	while (!s->onexit) {
		res = select(usocketfd + 1, &set, NULL, NULL, NULL);
		syncsd_debug("select return %d, errno %d, error[%s]", res, errno, strerror(errno));
		if (res == 0) continue;
		if ((res == -1) && (errno == EINTR)) {
			continue;
		}

		addr_len = sizeof(struct sockaddr_in);
		read_size = recvfrom(usocketfd, buffer, SYNCS_CLIENT_BUFFER_SIZE, 0, (struct sockaddr *) &addr, &addr_len);
		if (read_size < (int) sizeof(struct syncs_header)) {
			if (read_size == -1) {
				if ((errno == EAGAIN) || (errno == EINTR))
					continue;
				syncsd_debug("exit with %i", errno);
				break;
			}
			if (read_size == 0) {
				break;
			}
			continue;
		}
		packet_header = (struct syncs_header *) (buffer);
		if (packet_header->magic != SYNCS_PACKET_MAGIC) {
			continue;
		}
		if (packet_header->magic_data != SYNCS_PACKET_MAGIC_DATA) {
			continue;
		}
		packet_header->data_size &= SYNCS_VARIABLE_SIZE_MAXIMUM;
		syncs_process_packet(s, (struct syncs_packet *)packet_header);
	}
	s->ready = 0;

}

void *syncs_udpconnect_thread(void *server)
{
	struct syncs_connect *s = server;

	while ((s->addr[0] == 0) || (s->port == 0)) {
		if (!syncs_find_server(s->addr, &s->port))
			break;
		syncsd_info("Still looking for tcp server");
	}
	syncsd_debug("start client thread");
	while (!s->onexit) {
		s->usocketfd = syncs_udpclient_open(s->addr, s->port, &s->saddr);
		if (s->usocketfd < 0) {
			// syncsd_error("could't open socket");
			usleep(300000);
			continue;
		}
		syncs_set_nonblocking_socket(s->usocketfd, 1024 * 1024, 1024 * 1024);
		fcntl(s->usocketfd, F_SETFD, FD_CLOEXEC);
		syncs_udprecv(s);
		close(s->usocketfd);
		s->usocketfd = -1;
		usleep(1000000);
	}
	return NULL;
}

int syncs_connect_wait(struct syncs_connect *s, unsigned int timeout_sec)
{
	return syncs_wait_for(&s->connect_wait, &s->connect_mutex, &s->connect_cond, timeout_sec);
}


static void syncs_connect_mutex_init(pthread_mutex_t *mutex, pthread_cond_t *cond, pthread_condattr_t *attr)
{
	pthread_mutex_init(mutex, NULL);
	pthread_cond_init(cond, attr);
}


static void syncs_connect_structure_init(struct syncs_connect * s)
{
	int i;
	pthread_condattr_t attr;

	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

	syncs_connect_mutex_init (&s->read_mutex, &s->read_cond, &attr);
	syncs_connect_mutex_init (&s->event_wait_mutex, &s->event_wait_cond, &attr);
	syncs_connect_mutex_init (&s->eventlist_mutex, &s->eventlist_cond, &attr);
	syncs_connect_mutex_init (&s->channellist_mutex, &s->channellist_cond, &attr);
	syncs_connect_mutex_init (&s->clientlist_mutex, &s->clientlist_cond, &attr);
	syncs_connect_mutex_init (&s->ticket_mutex, &s->ticket_cond, &attr);
	syncs_connect_mutex_init (&s->connect_mutex, &s->connect_cond, &attr);

	s->socketfd = -1;
	s->usocketfd = -1;
	s->connect_cb = NULL;
	s->connect_arg = NULL;
	s->onexit = 0;
	s->port = 0;
	s->addr[0] = 0;
	s->saddr_size = sizeof(struct sockaddr_in);
	s->connect_wait = 1;
	s->event_wait = 1;
	s->current_key = NULL;

	syncsd_debug("init structure");
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++) {
		s->events[i].id.i[0] = -1;
		s->events[i].data = NULL;
		pthread_mutex_init(&(s->events[i].data_mutex), NULL);
	}
	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++) {
		s->channels[i].id.i[0] = -1;
	}
}

struct syncs_connect *syncs_udpconnect(const char *addr, int port, const char *id)
{
	struct syncs_connect *s;

	if ((s = calloc(1, sizeof(struct syncs_connect))) == NULL)
		return NULL;
	syncs_connect_structure_init(s);

	if (addr != NULL) {
		strncpy(s->addr, addr, 20);
		s->port = port;
	}
	if (id != NULL)
		syncs_idstr(&s->id, id);
	syncsd_debug("connecting to %s:%i create thread", addr, port);
	pthread_create(&s->thread, NULL, &syncs_udpconnect_thread, (void*) s);
	return s;
}

struct syncs_connect *syncs_connect(const char *addr, int port, const char *id, void (*cb)(void *), void *arg)
{
	struct syncs_connect *s;

	syncsd_debug("allocating memory for client SyncScribe structure: %lu Kb", sizeof(struct syncs_connect) / 1024);
	if ((s = calloc(1, sizeof(struct syncs_connect))) == NULL)
		return NULL;
	syncs_connect_structure_init(s);

	if (addr != NULL) {
		strncpy(s->addr, addr, 20);
		s->port = port;
	}
	if (id != NULL)
		syncs_idstr(&s->id, id);
	s->connect_cb = cb;
	s->connect_arg = arg;
	syncsd_debug("connecting to %s:%i create thread with data %p", addr, port, s);
	pthread_create(&s->thread, NULL, &syncs_connect_thread, (void*) s);
	return s;
}

struct syncs_connect *syncs_connect_simple(const char *addr, int port, const char *id)
{
	return syncs_connect(addr, port, id, NULL, NULL);
}


static int syncs_event_data_release(struct syncs_connect *s)
{
	int i;
	for (i = 0; i < SYNCS_EVENT_MAXIMUM; i++) {
		if (s->events[i].data !=  NULL) {
			free (s->events[i].data);
			s->events[i].data =  NULL;
		}
	}
	return 0;
}


static int syncs_wait_for_clientlist(struct syncs_connect *s, int timeout)
{
	return syncs_wait_for(&s->clientlist_wait, &s->clientlist_mutex, &s->clientlist_cond, timeout);
}

struct syncs_client_info *syncs_request_clientslist(struct syncs_connect *s, uint32_t *count,unsigned int timeout)
{
	struct syncs_header packet;


	if (s->clients_info == NULL) {
		s->clients_info = calloc(SYNCS_CLIENT_MAXIMUM, sizeof(struct syncs_client_info));
		if (s->clients_info == NULL) {
			return NULL;
		}
	}
	if (s->socketfd < 0) {
		*count = 0;
		return NULL;
	}

	packet.id.c[0] = ++s->clientlist_sequence;
	syncs_fill_basic_header_request (&packet, SYNCS_TYPE_CLIENT_LIST);
	s->clientlist_recv = 0;
	s->clientlist_wait_packet = 0;
	s->clientlist_wait = 1;

	syncs_connect_send(s, &packet, sizeof(struct syncs_header));
	syncsd_debug("request clients info");

	if (syncs_wait_for_clientlist(s, timeout))
		return NULL;

	*count = s->clientlist_recv;
	return s->clients_info;
}

void syncs_free_clientslist(struct syncs_connect *s)
{
	if (s->clients_info != NULL) {
		free(s->clients_info);
		s->clients_info = NULL;
	}
}

int syncs_wait_for_eventlist(struct syncs_connect *s, int timeout)
{
	struct timespec to;

	clock_gettime(CLOCK_MONOTONIC, &to);
	to.tv_sec += timeout;

	pthread_mutex_lock(&s->eventlist_mutex);
	while (s->eventlist_wait) {
		syncsd_debug("wait for events list");
		if (pthread_cond_timedwait(&s->eventlist_cond, &s->eventlist_mutex, &to) == ETIMEDOUT) {
			pthread_mutex_unlock(&s->eventlist_mutex);
			return -ETIMEDOUT;
		}
	}
	pthread_mutex_unlock(&s->eventlist_mutex);
	return 0;
}

struct syncs_event_info *syncs_request_eventslist(struct syncs_connect *s, uint32_t *count, unsigned int timeout)
{
	struct syncs_header packet;

	if (s->events_info == NULL) {
		s->events_info = calloc(SYNCS_EVENT_MAXIMUM, sizeof(struct syncs_event_info));
		if (s->events_info == NULL) {
			return NULL;
		}
	}

	if (s->socketfd < 0) {
		*count = 0;
		return NULL;
	}

	packet.id.c[0] = ++s->eventlist_sequence;
	syncs_fill_basic_header_request (&packet, SYNCS_TYPE_EVENT_LIST);
	s->eventlist_recv = 0;
	s->eventlist_wait_packet = 0;
	s->eventlist_wait = 1;

	syncs_connect_send(s, &packet, sizeof(struct syncs_header));
	syncsd_debug("request events info");

	if (syncs_wait_for_eventlist(s, timeout))
		return NULL;

	*count = s->eventlist_recv;
	return s->events_info;
}

void syncs_free_eventslist(struct syncs_connect *s)
{
	if (s->events_info != NULL) {
		free(s->events_info);
		s->events_info = NULL;
	}
}

int syncs_isconnect(struct syncs_connect *s)
{
	syncsd_debug("data ptr %p", s);
	if ((s == NULL) || (s->ready == 0)) return 0;
	else
		return 1;
}

int syncs_connect_status(struct syncs_connect *s, char *id)
{
	syncsd_debug("data ptr %p", s);
	if ((s == NULL) || (s->ready == 0)) return -1;
	else {
		syncs_stridcpy(id, &s->server_id);
		return 0;
	}
}

struct syncs_connect_channel *syncs_find_free_channel(struct syncs_connect *s)
{
	int i;

	for (i = 0; i < SYNCS_CHANNEL_MAXIMUM; i++)
		if (s->channels[i].id.i[0] == -1) {
			return &s->channels[i];
		}
	return NULL;
}

int syncs_wait_for_ticket(struct syncs_connect *s, int timeout)
{
	return syncs_wait_for(&s->ticket_wait, &s->ticket_mutex, &s->ticket_cond, timeout);
}

int syncs_channel_anons(struct syncs_connect *s, const char *id, uint32_t flags, int port)
{
	struct syncs_connect_channel *c;

	c = syncs_find_free_channel(s);
	c->ticket.ip = 0;
	c->ticket.port = port;
	c->ticket.flags = flags;
	syncs_idstr(&c->id, id);

	if (s->socketfd >= 0)
		syncs_client_send_channel_anons(s, &c->id, &c->ticket);

	return 0;
}

int syncs_channel_request(struct syncs_connect *s, const char *id, struct syncs_channel_ticket *ticket)
{
	struct syncs_packet packet;
	uint32_t size = sizeof(struct syncs_header);

	if (s->socketfd < 0)
		return -EBADFD;

	syncs_fill_header_request_str (&packet.header, id, SYNCS_TYPE_CHANNEL | SYNCS_CHANNEL_REQUEST);
	s->ticket_id = packet.header.id;

	syncs_connect_send(s, &packet, size);
	syncsd_debug("sent request");
	if (syncs_wait_for_ticket(s, 3))
		return -ETIMEDOUT;

	memcpy(ticket, &s->ticket_data, sizeof(struct syncs_channel_ticket));

	if (ticket->ip == htonl(INADDR_LOOPBACK))
		ticket->ip = inet_addr(s->addr);

	return 0;
}

static int syncs_wait_for_channellist(struct syncs_connect *s, int timeout)
{
	return syncs_wait_for(&s->channellist_wait, &s->channellist_mutex, &s->channellist_cond, timeout);
}

struct syncs_channel_info *syncs_request_channelslist(struct syncs_connect *s, uint32_t *count, unsigned int timeout)
{
	struct syncs_header packet;
	int i;

	if (s->channels_info == NULL) {
		s->channels_info = calloc(SYNCS_CHANNEL_MAXIMUM, sizeof(struct syncs_channel_info));
		if (s->channels_info == NULL) {
			return NULL;
		}
	}

	if (s->socketfd < 0) {
		*count = 0;
		return NULL;
	}

	packet.id.c[0] = ++s->channellist_sequence;
	syncs_fill_basic_header_request (&packet, SYNCS_TYPE_CHANNEL_LIST);
	s->channellist_recv = 0;
	s->channellist_wait_packet = 0;
	s->channellist_wait = 1;
	syncs_connect_send(s, &packet, sizeof(struct syncs_header));
	syncsd_debug("request channels info");

	if (syncs_wait_for_channellist(s, timeout))
		return NULL;

	for (i = 0; i < s->channellist_recv; i++) {
		if ((s->channels_info[i].ip == htonl(INADDR_LOOPBACK)) && (s->addr != NULL) && (*s->addr != 0))
			s->channels_info[i].ip = inet_addr(s->addr);
	}
	*count = s->channellist_recv;
	return s->channels_info;

}

void syncs_free_channelslist(struct syncs_connect *s)
{
	if (s->channels_info != NULL) {
		free(s->channels_info);
		s->channels_info = NULL;
	}
}

int syncs_ssdp_msearch(char *buffer, uint32_t size)
{
	return(snprintf(buffer, size, "%sHOST:%s:%d\r\nMAN:\"ssdp:discover\"\r\nMX:1\r\nST:%s\r\nUSER-AGENT:unknow\r\n\r\n",
		ssdp_headers.msearch, ssdp_network.ip, ssdp_network.port, syncs_ssdp_field.name));
}

int syncs_find_server(char *addr, int *port)
{
	int fd;
	struct sockaddr_in gaddr;
	struct sockaddr_in ssdp_server_addr;
	socklen_t ssdp_server_addr_len = sizeof(struct sockaddr_in);
	char ssdp_buffer[SSDP_PACKET_SIZE];
	int packet_size = 0;
	int ssdp_response_size = strlen(ssdp_headers.response);
	char *location;
	int addr_count = 0;
	int _port = 0;

	fd = syncs_udpmulticast_open(NULL, ssdp_network.port);
	syncs_add_multicast_group(fd, ssdp_network.ip, NULL);
	syncs_set_multicast_group(&gaddr, ssdp_network.ip, ssdp_network.port);
	syncs_set_rxtimeout(fd, 0, 500000);

	packet_size = syncs_ssdp_msearch(ssdp_buffer, SSDP_PACKET_SIZE);
	sendto(fd, ssdp_buffer, packet_size, 0, (struct sockaddr *) &gaddr, sizeof(struct sockaddr_in));

	while (1) {
		syncsd_debug("wait for ssdp packet");
		packet_size = recvfrom(fd, ssdp_buffer, SSDP_PACKET_SIZE, 0, (struct sockaddr *) &ssdp_server_addr, &ssdp_server_addr_len);
		if (packet_size < 0) break;
		if (packet_size < ssdp_response_size) continue;
		if (memcmp(ssdp_buffer, ssdp_headers.response, ssdp_response_size))
			continue;
		if (strstr(ssdp_buffer, syncs_ssdp_field.name) == NULL)
			continue;
		location = strstr(ssdp_buffer, "LOCATION:");
		if (location == NULL)
			continue;
		while (*location != ':') location++;
		location++;
		addr_count = 0;
		while (*location != ':') {
			*addr++ = *location++;
			addr_count++;
			if (addr_count > INET_ADDRSTRLEN)
				break;
		}
		location++;
		while (isdigit(*location)) {
			_port *= 10;
			_port += *location - '0';
			location++;
		}
		*port = _port;
		inet_ntop(AF_INET, &(ssdp_server_addr.sin_addr), addr, INET_ADDRSTRLEN);
		syncs_udpserver_close(fd);
		return 0;
	}
	syncs_udpserver_close(fd);
	return -1;
}

void syncs_disconnect(struct syncs_connect *s)
{
	s->onexit = 1;
	if (s->connect_cb_status == 1)
		pthread_cancel(s->connect_thread);
	shutdown(s->usocketfd, SHUT_WR);
	shutdown(s->socketfd, SHUT_WR);
	pthread_join(s->thread, NULL);
	syncs_free_clientslist(s);
	syncs_free_eventslist(s);
	syncs_free_channelslist(s);
	syncs_event_data_release(s);
	free(s);
}

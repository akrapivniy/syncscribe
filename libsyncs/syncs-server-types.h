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

#ifndef __SYNCS_SERVER_TYPES__
#define __SYNCS_SERVER_TYPES__

#ifdef __cplusplus
extern "C" {
#endif

#include "syncs-types.h"


struct syncs_epoll_cb {
	void *socket;
	int (*cb)(void *, uint32_t);
};

struct syncs_client {
	syncsid_t id;
	int socketfd;
	struct syncs_epoll_cb epoll_data;
	struct syncs_server *server;
	struct sockaddr_in addr;
	int addr_size;
	int event_subscribe;
	int event_write;
	int rx_event_count;
	int tx_event_count;
        int tx_error;
	int version;
	uint8_t buffer[SYNCS_CLIENT_BUFFER_SIZE];
	uint32_t buffer_recv;
	uint8_t key[SYNCS_CRYPT_KEY_SIZE];
};


struct syncs_event {
	syncsid_t id;
	char data[SYNCS_VARIABLE_SIZE_MAXIMUM];
	uint32_t data_type;
	uint32_t data_size;
	uint32_t count;
	uint64_t update_counter;
	uint32_t consumers_count;
	uint32_t producers_count;
	struct syncs_client *consumers[SYNCS_CLIENT_MAXIMUM];
	struct syncs_client *producer;
	void (*cb)(void *, char *, void *, uint32_t);
	void *args;
};

struct syncs_channel {
	syncsid_t id;
	struct syncs_channel_ticket ticket;
	uint32_t anons_count;
	uint32_t request_count;
	uint32_t producers_count;
	struct syncs_client *producer;
};

struct syncs_server {
	syncsid_t id;
	char addr[20];
	int port;
	int socketfd;
	int epollfd;
	uint8_t key[SYNCS_CRYPT_KEY_SIZE];
	struct syncs_epoll_cb epoll_data;
        struct syncs_epoll_cb epoll_udpdata;
	struct epoll_event socket_events[SYNCS_CLIENT_MAXIMUM];
	pthread_t thread;
	struct syncs_event events[SYNCS_EVENT_MAXIMUM];
	struct syncs_client clients[SYNCS_CLIENT_MAXIMUM];
	struct syncs_channel channels[SYNCS_CHANNEL_MAXIMUM];
	uint32_t channel_count;
	uint32_t event_count;
	uint32_t client_count;
	int sync_offset;
	uint64_t update_counter;

	int usocketfd;
	int uepollfd;
	struct syncs_epoll_cb uepoll_data;
        uint8_t ubuffer[SYNCS_CLIENT_BUFFER_SIZE];
	uint8_t crypt_buffer[SYNCS_VARIABLE_SIZE_MAXIMUM+16];
	int uclient_count;

	int ssdp_socketfd;
	pthread_t ssdp_thread;
        int ssdp_beacon;
};

#ifdef __cplusplus
}
#endif

#endif //__SYNCS_SERVER_TYPES__

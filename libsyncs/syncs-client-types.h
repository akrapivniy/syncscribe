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
#ifndef __SYNCS_CLIENT_TYPES__
#define __SYNCS_CLIENT_TYPES__

#ifdef __cplusplus
extern "C" {
#endif

#include "syncs-types.h"

	struct syncs_client_event {
		syncsid_t id;
		void (*cb)(void *, char *, void *, uint32_t);
		void *args;
		uint64_t update_counter;
		int flags;
		uint8_t *data;
		uint32_t data_size;
		uint32_t data_user_size;
		pthread_mutex_t data_mutex;
	};

	struct syncs_connect_channel {
		syncsid_t id;
                struct syncs_channel_ticket ticket;
	};

	struct syncs_connect {
		syncsid_t id;
		syncsid_t server_id;

		char addr[20];
		int port;
		int socketfd;
		int usocketfd;
		uint8_t buffer[SYNCS_CLIENT_BUFFER_SIZE];
		pthread_t thread;
		uint8_t read_data[SYNCS_VARIABLE_SIZE_MAXIMUM];
		int onexit;
		int ready;
		int connect_wait;
		pthread_mutex_t connect_mutex;
		pthread_cond_t connect_cond;

		int read_size;
		syncsid_t read_id;
		int read_wait;
		pthread_mutex_t read_mutex;
		pthread_cond_t read_cond;
                struct sockaddr_in saddr;
                int saddr_size;

		pthread_mutex_t event_wait_mutex;
		pthread_cond_t event_wait_cond;
		int event_wait;

		struct syncs_channel_ticket ticket_data;
		syncsid_t ticket_id;
		int ticket_wait;
		pthread_mutex_t ticket_mutex;
		pthread_cond_t ticket_cond;

		struct syncs_client_event events[SYNCS_EVENT_MAXIMUM];
		struct syncs_client_event *events_queue;

		struct syncs_connect_channel channels[SYNCS_CHANNEL_MAXIMUM];
		struct syncs_event_info *events_info;
		struct syncs_client_info *clients_info;
		struct syncs_channel_info *channels_info;

		uint8_t *current_key;
		uint8_t server_key [SYNCS_CRYPT_KEY_SIZE];
		uint8_t session_key [SYNCS_CRYPT_KEY_SIZE];

		pthread_t connect_thread;
		int connect_cb_status;
		void (*connect_cb)(void *);
		void *connect_arg;

		char eventlist_sequence;
		char eventlist_wait_packet;
		int eventlist_recv;
		int eventlist_wait;
		pthread_mutex_t eventlist_mutex;
		pthread_cond_t eventlist_cond;

		char clientlist_sequence;
		char clientlist_wait_packet;
		int clientlist_recv;
		int clientlist_wait;
		pthread_mutex_t clientlist_mutex;
		pthread_cond_t clientlist_cond;

		char channellist_sequence;
		char channellist_wait_packet;
		int channellist_recv;
		int channellist_wait;
		pthread_mutex_t channellist_mutex;
		pthread_cond_t channellist_cond;
	};


#ifdef __cplusplus
}
#endif



#endif //__SYNCS_CLIENT_TYPES__

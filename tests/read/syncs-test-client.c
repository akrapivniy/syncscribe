/**************************************************************
 * Description: Utility and test tools to support SyncScribe library
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
#include <syncs-client.h>
#include <stdint.h>
#include <unistd.h>
#include <libconfig.h>
#include <time.h>

#define MODULE_NAME "syncs-test-client"
#include <syncs-debug.h>

void server_cb(void *args, char *id, void *data, uint32_t size)
{
	struct timespec time;
	struct syncs_connect *s = args;
	int client_count = 0;

	clock_gettime(CLOCK_REALTIME, &time);
	syncsd_debug("event %d from client at %lu:%lu", *(int *)data, time.tv_sec, time.tv_nsec);

	syncs_write(s, SYNCS_TYPE_VAR_INT32, "client_count", &client_count, 4);
	syncs_write(s, SYNCS_TYPE_VAR_INT32 | SYNCS_TYPE_ECHO, "client_echo_count", &client_count, 4);
	client_count++;
}

void client_echo_cb(void *args, char *id, void *data, uint32_t size)
{
	syncsd_debug("server gets event");
}

void client_cb(void *args, char *id, void *data, uint32_t size)
{
	syncsd_error("CALLBACK is called!");
}

int main()
{
	config_t cfg;
	const char *ipaddress;
	char *default_ipaddress = "127.0.0.1";
	struct syncs_connect *s;
	int server_count;
	int i;
	char addr[20];
	int port;


	config_init(&cfg);

	if (!config_read_file(&cfg, "client.config")) {
		syncsd_error("%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
	} else {
		if (!config_lookup_string(&cfg, "ipaddress", &ipaddress))
			ipaddress = default_ipaddress;
	}
	syncs_find_server(addr, &port);
	syncsd_debug("connecting to %s:%d", addr, port);

	while (1) {
		syncsd_debug("connecting to %s", ipaddress);
		s = syncs_connect((char *) ipaddress, 4444, "client1", NULL, NULL);
		syncs_subscribe_event(s, SYNCS_TYPE_VAR_INT32, "server_count", server_cb, s);
		syncs_subscribe_event(s, SYNCS_TYPE_VAR_INT32, "client_echo_count", client_echo_cb, s);
		syncs_subscribe_event(s, SYNCS_TYPE_VAR_INT32, "client_count", client_cb, s);
		syncs_channel_anons(s, "debug", 0, 5555);

		for (i = 0; i < 10; i++) {
			syncs_read_int32(s, 0, "server_count", &server_count);
			syncsd_debug("read value %d", server_count);
			sleep(1);
		}
		syncs_disconnect(s);
	}

	config_destroy(&cfg);
}

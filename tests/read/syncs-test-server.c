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
#include <syncs-server.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define MODULE_NAME "syncs-test-server"
#include <syncs-debug.h>


struct timespec client_time[8];
int client_number = 0;
int client_count = 0;
int server_count = 0;
int32_t server_count_copy = 0;

struct client_data {
	int count;
	struct timespec time;
};

void speed_cb(void *args, char *id, void *data, uint32_t size)
{
	struct client_data *cd = data;
	int i;

	if (size != sizeof(struct client_data)) {
		syncsd_error("event data");
	}
	syncsd_debug("data %d", cd->count);
	if (cd->count > client_count) {
		printf("%02d(%02d) -", client_count, client_number);
		for (i = 1; i < client_number; i++) {
			printf("%d[%6ld.%6ld] ", i, (client_time[0].tv_sec - client_time[i].tv_sec), (client_time[0].tv_nsec - client_time[i].tv_nsec) / 1000);
		}
		printf("-\n");
		client_count = cd->count;
		client_number = 0;
	}
	if (client_count != cd->count) return;
	client_time[client_number & 7] = cd->time;
	client_number = (client_number + 1)&0x07;
}

int main()
{
	struct syncs_server *s;

	syncsd_info("creating server...");
	s = syncs_server_create(NULL, 4444, "test");
	if (s == NULL) {
		syncsd_error("server create");
		return -1;
	}
	syncs_server_ssdp_create (s, "syncs.com", 1);
	syncsd_info("defining variables...");
	syncs_server_define(s, "client_count", SYNCS_TYPE_VAR_INT32, NULL, 0);
	syncs_server_define(s, "client_echo_count", SYNCS_TYPE_VAR_INT32, NULL, 0);
	syncs_server_define(s, "server_count", SYNCS_TYPE_VAR_INT32, NULL,0);
	syncs_server_define(s, "012345678901234567890123456789123456789012345", SYNCS_TYPE_VAR_INT32, NULL,0);


	syncsd_info("subscribing enent...");
	syncs_server_subscribe_event(s, 0, "client_count", speed_cb, s);
	syncs_server_subscribe_event(s, 0, "012345678901234567890123456789123456789012345", speed_cb, s);
	syncsd_info("main loop...");
	while (1) {
//		syncs_server_print_event (s);
		syncs_server_write(s, SYNCS_TYPE_VAR_INT32, "server_count", &server_count, 0);
		syncs_server_write_int32 (s, 0, "server_count", server_count_copy);
//		syncsd_info("server count %d : %d", server_count, server_count_copy);
		server_count++;
		usleep(500000);
	}


	syncs_server_stop(s);
}

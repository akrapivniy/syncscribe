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
#include <time.h>

#define MODULE_NAME "syncs-test-monitor"
#include <syncs-debug.h>

int main()
{
	struct syncs_connect *s;
	unsigned int count;
	struct syncs_event_info *events_info;
	struct syncs_client_info *clients_info;
	int i;
	char str[15];

	s = syncs_connect_simple(NULL, 0, "syncs-test-monitor");

	while (1) {
		events_info = syncs_request_eventslist(s, &count, 5);
		if (events_info == NULL) continue;
		printf("Event statistics\n");
		printf("|%15s|%15s|%7s|%7s|%7s\n", "id", "value", "count", "prod.", "cons.");

		for (i = 0; i < count; i++) {
			switch (events_info[i].type) {
			case SYNCS_TYPE_VAR_INT32: snprintf(str, 15, "%d", *(int *) events_info[i].short_data);
				break;
			case SYNCS_TYPE_VAR_INT64: snprintf(str, 15, "%ld", *(long *) events_info[i].short_data);
				break;
			case SYNCS_TYPE_VAR_FLOAT: snprintf(str, 15, "%f", *(float *) events_info[i].short_data);
				break;
			case SYNCS_TYPE_VAR_DOUBLE: snprintf(str, 15, "%lf", *(double *) events_info[i].short_data);
				break;
			case SYNCS_TYPE_VAR_STRING: snprintf(str, 15, "%s", (char *) events_info[i].short_data);
				break;
			default: snprintf(str, 10, "not support");
				break;
			}
			printf("|%15s|%15s|%7d|%7d|%7d\n", (char *) &events_info[i].id, str, events_info[i].count, events_info[i].producers_count, events_info[i].consumers_count);
		}

		clients_info = syncs_request_clientslist(s, &count, 5);
		if (clients_info == NULL) continue;
		printf("Client statistics\n");
		printf("|%15s|%7s|%7s|%7s|%7s\n", "id", "rx pkt", "tx pkt", "subscr", "write");
		for (i = 0; i < count; i++) {
			printf("|%15s|%7d|%7d|%7d|%7d\n", (char *) &clients_info[i].id, clients_info[i].rx_event_count, clients_info[i].tx_event_count, clients_info[i].event_subscribe, clients_info[i].event_write);
		}
		sleep(1);
	}

}

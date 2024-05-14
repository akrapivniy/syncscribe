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

#define MODULE_NAME "syncs-test-sync-client"
#include <syncs-debug.h>
#include <test_tools.h>



int main()
{
	struct syncs_connect *s = NULL;

	s = syncs_connect_simple(NULL, 0, "sync-test-event-client");
	if (s == NULL) {
		syncsd_debug("error due connection");
		return -1;
	}
	syncs_subscribe_event_sync(s, SYNCS_TYPE_VAR_INT32, "client_count");
	syncs_subscribe_event_sync(s, SYNCS_TYPE_VAR_STRING, "client_string");
	syncs_connect_wait (s, 0);

	while (1) {
		uint8_t data[1200];
		uint32_t flag;
		int32_t *value_int = (int32_t *	)data;
		uint32_t data_size = 1200;
		const char *id = syncs_wait_event(s, &flag, &data, &data_size, 1);
		if (id == NULL)
			continue;

		if ((flag & SYNCS_TYPE_VAR_MASK) == SYNCS_TYPE_VAR_INT32)
			printf ("New 0x%08x event ID %s Data size %u Value %d  \n ", flag, id, data_size, *value_int);
		else if ((flag & SYNCS_TYPE_VAR_MASK) == SYNCS_TYPE_VAR_STRING)
			printf ("New 0x%08x event ID %s Data size %u Data: %s  \n ", flag, id, data_size, data);

	}
	syncs_disconnect(s);
}

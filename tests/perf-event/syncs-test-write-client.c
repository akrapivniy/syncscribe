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

int main()
{
	struct syncs_connect *s;
	int32_t client_count;
	char buffer[1024];
	int i;


	for (i = 0; i < 1023; i++) {
		buffer[i] = '0' + i % 10;
	}
	buffer[1023] = 0;

	s = syncs_connect_simple(NULL, 0, "syncs-test-write-client");
//	s = syncs_find_udpconnect("client1", 0);
	if (s == NULL) {
		syncsd_debug("error due connection");
		return -1;
	}
	syncs_connect_wait(s, 0);

	while (1) {
		while (syncs_write_int32(s, 0, "client_count", client_count));
		while (syncs_write_str(s, 0, "client_string", buffer));
		client_count++;
	}
	syncs_disconnect(s);
}

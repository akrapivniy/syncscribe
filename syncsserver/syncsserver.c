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
#include <libconfig.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <json-c/json.h>

#define MODULE_NAME "syncs-server"
#include <syncs-debug.h>
#include <syncs-server.h>
#include <syncs-client.h>

struct syncs_control {
	struct syncs_server *server;
	struct syncs_connect *client;
};

struct syncs_server *syncs_json_server_init(char *filename)
{
	struct syncs_server *server = NULL;
	struct json_object *parsed_json;
	struct json_object *name;
	struct json_object *address;
	struct json_object *port;
	struct json_object *ssdp;
	struct json_object *variables;

	const char *server_name;
	const char *server_address;
	int server_port;
	const char *server_ssdp_name;
	int n_variables;
	int i;

	parsed_json = json_object_from_file(filename);
	if (!parsed_json) return NULL;

	if (json_object_object_get_ex(parsed_json, "server_name", &name)) {
		server_name = json_object_get_string(name);
	}

	if (json_object_object_get_ex(parsed_json, "address", &address)) {
		server_address = json_object_get_string(address);
	}

	if (json_object_object_get_ex(parsed_json, "port", &port)) {
		server_port = json_object_get_int(port);
	}

	server = syncs_server_create(server_address, server_port, server_name);
	if (server == NULL)
		return NULL;

	if (json_object_object_get_ex(parsed_json, "ssdp", &ssdp)) {
		server_ssdp_name = json_object_get_string(ssdp);
		syncs_server_ssdp_create (server, server_ssdp_name, 1);
	}

	json_object_object_get_ex(parsed_json, "variables", &variables);
	n_variables = json_object_array_length(variables);

	for (i = 0; i < n_variables; i++) {
		struct json_object *variable = NULL;
		struct json_object *name = NULL;
		struct json_object *type = NULL;
		struct json_object *value = NULL;
		struct json_object *size = NULL;
		int size_int = 0;
		int flags = 0;

		if ((variable = json_object_array_get_idx(variables, i)) == NULL) continue;

		if (!json_object_object_get_ex(variable, "name", &name)) continue;
		if (!json_object_object_get_ex(variable, "type", &type)) continue;
		json_object_object_get_ex(variable, "value", &value);
		if (json_object_object_get_ex(variable, "size", &size)) {
			size_int = json_object_get_int(size);
		}

		const char *type_string = json_object_get_string(type);
		const char *name_string = json_object_get_string(name);

		if (!strcmp(type_string, "empty"))
			flags = SYNCS_TYPE_VAR_EMPTY;
		else if (!strcmp(type_string, "int"))
			flags = SYNCS_TYPE_VAR_INT32;
		else if (!strcmp(type_string, "long"))
			flags = SYNCS_TYPE_VAR_INT64;
		else if (!strcmp(type_string, "float"))
			flags = SYNCS_TYPE_VAR_FLOAT;
		else if (!strcmp(type_string, "double"))
			flags = SYNCS_TYPE_VAR_DOUBLE;
		else if (!strcmp(type_string, "string"))
			flags = SYNCS_TYPE_VAR_STRING;
		else if (!strcmp(type_string, "struct"))
			flags = SYNCS_TYPE_VAR_STRUCTURE;
		else if (!strcmp(type_string, "any"))
			flags = SYNCS_TYPE_VAR_ANY;
		else
			flags = SYNCS_TYPE_VAR_NOT_DEFINED;

		switch (flags) {
		case SYNCS_TYPE_VAR_INT32:
		case SYNCS_TYPE_VAR_INT64:
			if (value != NULL) {
				int value_int = json_object_get_int(value);
				syncsd_info("register %d variable %s type int size %d value %d ", i, name_string, 4, value_int);
				syncs_server_define(server, name_string, flags, &value_int, 8);
				continue;
			}
			break;
		case SYNCS_TYPE_VAR_STRING:
			if (value != NULL) {
				const char *value_string = json_object_get_string(value);
				int size_string = strlen(value_string);
				syncsd_info("register %d variable %s type string size %d value %s", i, name_string, size_string, value_string);
				syncs_server_define(server, name_string, flags, (void *)value_string, size_string);
				continue;
			}
			break;
		default:
			syncsd_info("register %d variable %s size %d value undefine", i, name_string, size_int);
			syncs_server_define(server, name_string, flags, NULL, size_int);
			break;
		}
	}

	json_object_put(parsed_json);
	return server;
}


struct syncs_connect *syncs_json_client_init(char *filename)
{
	struct syncs_connect *client = NULL;
	struct json_object *parsed_json;
	struct json_object *name;
	struct json_object *address;
	struct json_object *port;

	const char *client_name;
	const char *server_address;
	int server_port;

	parsed_json = json_object_from_file(filename);
	if (!parsed_json) return NULL;

	if (json_object_object_get_ex(parsed_json, "client_name", &name)) {
		client_name = json_object_get_string(name);
	}

	if (json_object_object_get_ex(parsed_json, "address", &address)) {
		server_address = json_object_get_string(address);
	}

	if (json_object_object_get_ex(parsed_json, "port", &port)) {
		server_port = json_object_get_int(port);
	}

	client = syncs_connect_simple(server_address, server_port, client_name);
	if (client == NULL)
		return NULL;

	json_object_put(parsed_json);
	return client;
}

void syncs_cb_message(void *args, char *id, void *data, uint32_t size)
{
	(void)args;
	(void)id;
	(void)data;
	(void)size;
}

void syncs_cb_mode(void *args, char *id, void *data, uint32_t size)
{
	(void)args;
	(void)id;
	(void)data;
	(void)size;
}

static struct option long_options[] = {
	{"config", required_argument, 0, 'c'},
	{"quite", no_argument, 0, 'q'},
	{0, 0, 0, 0}
};
static const char *short_options = "qc:";

int main(int argc, char** argv)
{
	struct syncs_control syncs;
	int bequite = 0;
	char server_config_filename[255] = "./syncscribe-server.json";
	int long_index;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, short_options,
		long_options, &long_index)) != -1) {
		switch (opt) {
		case 'c':
			strncpy(server_config_filename, optarg, 255);
			break;
		case 'q':
			bequite = 1;
			break;
		default:
			break;
		}
	}

	syncs.server = syncs_json_server_init(server_config_filename);
	syncs.client = syncs_json_client_init(server_config_filename);

	syncs_subscribe_event(syncs.client, SYNCS_TYPE_VAR_INT32, "mode", syncs_cb_mode, NULL);
	syncs_subscribe_event(syncs.client, SYNCS_TYPE_VAR_INT32, "pause", syncs_cb_mode, NULL);
	syncs_server_subscribe_event(syncs.server, SYNCS_TYPE_VAR_STRING, "message", syncs_cb_message, NULL);

	while (1) {
		if (!bequite)
			syncs_server_print_event(syncs.server, stdout);
		usleep(1000000);
	}

	return 0;
}

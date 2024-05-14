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

#ifndef __SYNCS_CLIENT__
#define __SYNCS_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <syncs-types.h>

/**
 * @brief Establishes a connection to the server.
 *
 * @param addr The server address.
 * @param port The server port.
 * @param id The client ID.
 * @param cb A callback function for connection events.
 * @param args Arguments for the callback function.
 * @return A pointer to the syncs_connect structure.
 */
struct syncs_connect *syncs_connect(const char *addr, int port, const char *id, void (*cb)(void *), void *args);

/**
 * @brief Establishes a simple connection to the server without a callback function.
 *
 * @param addr The server address.
 * @param port The server port.
 * @param id The client ID.
 * @return A pointer to the syncs_connect structure.
 */
struct syncs_connect *syncs_connect_simple(const char *addr, int port, const char *id);

/**
 * @brief Establishes a UDP connection to the server.
 *
 * @param addr The server address.
 * @param port The server port.
 * @param id The client ID.
 * @return A pointer to the syncs_connect structure.
 */
struct syncs_connect *syncs_udpconnect(const char *addr, int port, const char *id);

/**
 * @brief Finds a server and retrieves its address and port.
 *
 * @param addr The buffer to store the server address.
 * @param port The pointer to store the server port.
 * @return 0 on success, -1 on failure.
 */
int syncs_find_server(char *addr, int *port);

/**
 * @brief Checks if the connection is active.
 *
 * @param s The syncs_connect structure.
 * @return 1 if connected, 0 otherwise.
 */
int syncs_isconnect(struct syncs_connect *s);

/**
 * @brief Retrieves the connection status and server ID.
 *
 * @param s The syncs_connect structure.
 * @param server_id The buffer to store the server ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_connect_status(struct syncs_connect *s, char *server_id);

/**
 * @brief Waits for the connection to be established within the specified timeout.
 *
 * @param s The syncs_connect structure.
 * @param timeout_sec The timeout in seconds.
 * @return 0 on success, -1 on timeout.
 */
int syncs_connect_wait(struct syncs_connect *s, unsigned int timeout_sec);

/**
 * @brief Disconnects from the server.
 *
 * @param s The syncs_connect structure.
 */
void syncs_disconnect(struct syncs_connect *s);

/**
 * @brief Defines a new event or variable on the server.
 *
 * @param s The syncs_connect structure.
 * @param id The event or variable ID.
 * @param flags Additional flags for the definition.
 * @return 0 on success, -1 on failure.
 */
int syncs_define(struct syncs_connect *s, const char *id, uint32_t flags);

/**
 * @brief Undefines an existing event or variable.
 *
 * @param s The syncs_connect structure.
 * @param id The event or variable ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_undefine(struct syncs_connect *s, const char *id);

/**
 * @brief Subscribes to an event with a callback function for handling the event.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the subscription.
 * @param id The event ID.
 * @param cb The callback function.
 * @param args Arguments for the callback function.
 * @return 0 on success, -1 on failure.
 */
int syncs_subscribe_event(struct syncs_connect *s, uint32_t flags, const char *id, void (*cb)(void *, char *, void *, uint32_t), void *args);

/**
 * @brief Synchronously subscribes to an event.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the subscription.
 * @param id The event ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_subscribe_event_sync(struct syncs_connect *s, uint32_t flags, const char *id);

/**
 * @brief Synchronously subscribes to an event with user data.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the subscription.
 * @param id The event ID.
 * @param user_data User data to be passed with the subscription.
 * @param user_data_size Size of the user data.
 * @return 0 on success, -1 on failure.
 */
int syncs_subscribe_event_sync_user(struct syncs_connect *s, uint32_t flags, const char *id, void *user_data, uint32_t user_data_size);

/**
 * @brief Unsubscribes from an event.
 *
 * @param s The syncs_connect structure.
 * @param id The event ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_unsubscribe_event(struct syncs_connect *s, const char *id);

/**
 * @brief Reads data associated with an event or variable.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The buffer to store the data.
 * @param data_size The pointer to store the size of the data.
 * @return 0 on success, -1 on failure.
 */
int syncs_read(struct syncs_connect *s, uint32_t flags, const char *id, void *data, uint32_t *data_size);

/**
 * @brief Reads an int32 value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the int32 value.
 * @return 0 on success, -1 on failure.
 */
int syncs_read_int32(struct syncs_connect *s, uint32_t flags, const char *id, int32_t *data);

/**
 * @brief Reads an int64 value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the int64 value.
 * @return 0 on success, -1 on failure.
 */
int syncs_read_int64(struct syncs_connect *s, uint32_t flags, const char *id, int64_t *data);

/**
 * @brief Reads a float value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the float value.
 * @return 0 on success, -1 on failure.
 */
int syncs_read_float(struct syncs_connect *s, uint32_t flags, const char *id, float *data);

/**
 * @brief Reads a double value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the double value.
 * @return 0 on success, -1 on failure.
 */
int syncs_read_double(struct syncs_connect *s, uint32_t flags, const char *id, double *data);

/**
 * @brief Reads a string value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The buffer to store the string value.
 * @param size The size of the buffer.
 * @return 0 on success, -1 on failure.
 */
int syncs_read_str(struct syncs_connect *s, uint32_t flags, const char *id, char *data, uint32_t size);

/**
 * @brief Waits for an event to occur within the specified timeout.
 *
 * @param s The syncs_connect structure.
 * @param flags The pointer to store the event flags.
 * @param data The buffer to store the event data.
 * @param data_size The pointer to store the size of the event data.
 * @param timeout The timeout in seconds.
 * @return The event ID on success, NULL on timeout.
 */
const char *syncs_wait_event(struct syncs_connect *s, uint32_t *flags, void *data, uint32_t *data_size, int timeout);

/**
 * @brief Writes data associated with an event or variable.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The data to write.
 * @param data_size The size of the data.
 * @return 0 on success, -1 on failure.
 */
int syncs_write(struct syncs_connect *s, uint32_t flags, const char *id, void *data, uint32_t data_size);

/**
 * @brief Writes an int32 value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The int32 value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_int32(struct syncs_connect *s, uint32_t flags, const char *id, int32_t data);

/**
 * @brief Writes an int64 value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The int64 value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_int64(struct syncs_connect *s, uint32_t flags, const char *id, int64_t data);

/**
 * @brief Writes a float value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The float value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_float(struct syncs_connect *s, uint32_t flags, const char *id, float data);

/**
 * @brief Writes a double value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The double value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_double(struct syncs_connect *s, uint32_t flags, const char *id, double data);

/**
 * @brief Writes a string value.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The string value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_str(struct syncs_connect *s, uint32_t flags, const char *id, const char *data);

/**
 * @brief Writes an event.
 *
 * @param s The syncs_connect structure.
 * @param flags Additional flags for the write operation.
 * @param id The event ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_write_event(struct syncs_connect *s, uint32_t flags, const char *id);

/**
 * @brief Requests the list of connected clients.
 *
 * @param s The syncs_connect structure.
 * @param count The pointer to store the number of clients.
 * @param timeout The timeout in seconds.
 * @return A pointer to the syncs_client_info structure.
 */
struct syncs_client_info *syncs_request_clientslist(struct syncs_connect *s, uint32_t *count, unsigned int timeout);

/**
 * @brief Requests the list of events.
 *
 * @param s The syncs_connect structure.
 * @param count The pointer to store the number of events.
 * @param timeout The timeout in seconds.
 * @return A pointer to the syncs_event_info structure.
 */
struct syncs_event_info *syncs_request_eventslist(struct syncs_connect *s, uint32_t *count, unsigned int timeout);

/**
 * @brief Requests the list of channels.
 *
 * @param s The syncs_connect structure.
 * @param count The pointer to store the number of channels.
 * @param timeout The timeout in seconds.
 * @return A pointer to the syncs_channel_info structure.
 */
struct syncs_channel_info *syncs_request_channelslist(struct syncs_connect *s, uint32_t *count, unsigned int timeout);

/**
 * @brief Frees the memory allocated for the events list.
 *
 * @param s The syncs_connect structure.
 */
void syncs_free_eventslist(struct syncs_connect *s);

/**
 * @brief Frees the memory allocated for the clients list.
 *
 * @param s The syncs_connect structure.
 */
void syncs_free_clientslist(struct syncs_connect *s);

/**
 * @brief Frees the memory allocated for the channels list.
 *
 * @param s The syncs_connect structure.
 */
void syncs_free_channelslist(struct syncs_connect *s);

/**
 * @brief Announces a channel with the specified ID and port.
 *
 * @param s The syncs_connect structure.
 * @param id The channel ID.
 * @param flags Additional flags for the announcement.
 * @param port The channel port.
 * @return 0 on success, -1 on failure.
 */
int syncs_channel_anons(struct syncs_connect *s, const char *id, int flags, int port);

/**
 * @brief Requests a channel with the specified ID.
 *
 * @param s The syncs_connect structure.
 * @param id The channel ID.
 * @param ticket The pointer to store the channel ticket.
 * @return 0 on success, -1 on failure.
 */
int syncs_channel_request(struct syncs_connect *s, const char *id, struct syncs_channel_ticket *ticket);

#ifdef __cplusplus
}
#endif

#endif //__SYNCS_CLIENT__

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

#ifndef __SYNCS_SERVER__
#define __SYNCS_SERVER__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <syncs-types.h>

/**
 * @brief Creates a server instance with the specified address, port, and ID.
 *
 * @param addr The server address.
 * @param port The server port.
 * @param id The server ID.
 * @return A pointer to the created syncs_server structure.
 */
struct syncs_server *syncs_server_create(const char *addr, int port, const char *id);

/**
 * @brief Defines a new event or variable on the server.
 *
 * @param s The syncs_server structure.
 * @param id The event or variable ID.
 * @param type The type of the event or variable.
 * @param data The initial data for the event or variable.
 * @param size The size of the data.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_define(struct syncs_server *s, const char *id, int type, void *data, uint32_t size);

/**
 * @brief Undefines an existing event or variable on the server.
 *
 * @param s The syncs_server structure.
 * @param id The event or variable ID.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_undefine(struct syncs_server *s, const char *id);

/**
 * @brief Subscribes to an event on the server with a callback function.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the subscription.
 * @param id The event ID.
 * @param cb The callback function to handle the event.
 * @param args Arguments for the callback function.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_subscribe_event(struct syncs_server *s, int flags, const char *id, void (*cb)(void *, char *, void *, uint32_t), void *args);

/**
 * @brief Unsubscribes from an event on the server.
 *
 * @param s The syncs_server structure.
 * @param id The event ID.
 */
void syncs_server_unsubscribe_event(struct syncs_server *s, const char *id);

/**
 * @brief Reads data associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The buffer to store the data.
 * @param data_size The pointer to store the size of the data.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read(struct syncs_server *s, uint32_t flags, const char *id, void *data, uint32_t *data_size);

/**
 * @brief Reads a 32-bit integer value associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the 32-bit integer value.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read_int32(struct syncs_server *s, uint32_t flags, const char *id, int32_t *data);

/**
 * @brief Reads a 64-bit integer value associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the 64-bit integer value.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read_int64(struct syncs_server *s, uint32_t flags, const char *id, int64_t *data);

/**
 * @brief Reads a floating-point value associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the floating-point value.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read_float(struct syncs_server *s, uint32_t flags, const char *id, float *data);

/**
 * @brief Reads a double-precision floating-point value associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The pointer to store the double-precision floating-point value.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read_double(struct syncs_server *s, uint32_t flags, const char *id, double *data);

/**
 * @brief Reads a string value associated with an event or variable from the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the read operation.
 * @param id The event or variable ID.
 * @param data The buffer to store the string value.
 * @param size The size of the buffer.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_read_str(struct syncs_server *s, uint32_t flags, const char *id, char *data, uint32_t size);

/**
 * @brief Writes data associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The data to write.
 * @param data_size The size of the data.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write(struct syncs_server *s, uint32_t flags, const char *id, void *data, int data_size);

/**
 * @brief Writes a 32-bit integer value associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The 32-bit integer value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write_int32(struct syncs_server *s, uint32_t flags, const char *id, int32_t data);

/**
 * @brief Writes a 64-bit integer value associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The 64-bit integer value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write_int64(struct syncs_server *s, uint32_t flags, const char *id, int64_t data);

/**
 * @brief Writes a floating-point value associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The floating-point value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write_float(struct syncs_server *s, uint32_t flags, const char *id, float data);

/**
 * @brief Writes a double-precision floating-point value associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The double-precision floating-point value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write_double(struct syncs_server *s, uint32_t flags, const char *id, double data);

/**
 * @brief Writes a string value associated with an event or variable to the server.
 *
 * @param s The syncs_server structure.
 * @param flags Additional flags for the write operation.
 * @param id The event or variable ID.
 * @param data The string value to write.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_write_str(struct syncs_server *s, uint32_t flags, const char *id, const char *data);

/**
 * @brief Stops the server.
 *
 * @param s The syncs_server structure.
 */
void syncs_server_stop(struct syncs_server *s);

/**
 * @brief Sets the synchronization offset for the server.
 *
 * @param s The syncs_server structure.
 * @param ms The synchronization offset in milliseconds.
 */
void syncs_server_set_sync_offset(struct syncs_server *s, int ms);

/**
 * @brief Prints the event information to the specified stream.
 *
 * @param s The syncs_server structure.
 * @param stream The stream to print the event information to.
 */
void syncs_server_print_event(struct syncs_server *s, FILE *stream);

/**
 * @brief Creates an SSDP instance for the server.
 *
 * @param s The syncs_server structure.
 * @param address The SSDP address.
 * @param beacon The SSDP beacon.
 * @return 0 on success, -1 on failure.
 */
int syncs_server_ssdp_create(struct syncs_server *s, const char *address, int beacon);


#ifdef __cplusplus
}
#endif

#endif //__SYNCS_SERVER__

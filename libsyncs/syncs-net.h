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

#ifndef __SYNCS_NET_H__
#define __SYNCS_NET_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <arpa/inet.h>

/**
 * @brief Opens a TCP server socket at the specified address and port.
 *
 * @param addr The server address.
 * @param port The server port.
 * @return The socket file descriptor on success, -1 on failure.
 */
int syncs_tcpserver_open(const char *addr, int port);

/**
 * @brief Closes the specified TCP server socket.
 *
 * @param socketfd The socket file descriptor.
 */
void syncs_tcpserver_close(int socketfd);

/**
 * @brief Opens a UDP server socket at the specified address and port.
 *
 * @param addr The server address.
 * @param port The server port.
 * @return The socket file descriptor on success, -1 on failure.
 */
int syncs_udpserver_open(const char *addr, int port);

/**
 * @brief Closes the specified UDP server socket.
 *
 * @param socketfd The socket file descriptor.
 */
void syncs_udpserver_close(int socketfd);

/**
 * @brief Opens a UDP multicast socket at the specified address and port.
 *
 * @param addr The multicast address.
 * @param port The multicast port.
 * @return The socket file descriptor on success, -1 on failure.
 */
int syncs_udpmulticast_open(const char *addr, int port);

/**
 * @brief Opens a TCP client socket to the specified address and port.
 *
 * @param addr The server address.
 * @param port The server port.
 * @return The socket file descriptor on success, -1 on failure.
 */
int syncs_tcpclient_open(const char *addr, int port);

/**
 * @brief Closes the specified TCP client socket.
 *
 * @param socketfd The socket file descriptor.
 */
void syncs_tcpclient_close(int socketfd);

/**
 * @brief Opens a UDP client socket to the specified address and port.
 *
 * @param addr The server address.
 * @param port The server port.
 * @param serveraddr The server socket address structure to be filled.
 * @return The socket file descriptor on success, -1 on failure.
 */
int syncs_udpclient_open(const char *addr, int port, struct sockaddr_in *serveraddr);

/**
 * @brief Closes the specified UDP client socket.
 *
 * @param socketfd The socket file descriptor.
 */
void syncs_udpclient_close(int socketfd);

/**
 * @brief Sets the specified socket to non-blocking mode and configures the receive and transmit buffer sizes.
 *
 * @param socketfd The socket file descriptor.
 * @param rx_size The size of the receive buffer.
 * @param tx_size The size of the transmit buffer.
 */
void syncs_set_nonblocking_socket(int socketfd, int rx_size, int tx_size);

/**
 * @brief Sends data on a blocking socket.
 *
 * @param sock The socket file descriptor.
 * @param buffer The data buffer to send.
 * @param len The length of the data buffer.
 * @param flags The flags for the send operation.
 * @return The number of bytes sent on success, -1 on failure.
 */
int syncs_blocking_send(int sock, void *buffer, int len, int flags);

/**
 * @brief Sends data on a UDP socket.
 *
 * @param sock The socket file descriptor.
 * @param buffer The data buffer to send.
 * @param len The length of the data buffer.
 * @param saddr The destination socket address.
 * @param saddr_size The size of the destination socket address.
 * @return The number of bytes sent on success, -1 on failure.
 */
int syncs_udp_send(int sock, void *buffer, uint32_t len, struct sockaddr_in *saddr, uint32_t saddr_size);

/**
 * @brief Sets the keepalive parameters for the specified socket.
 *
 * @param socketfd The socket file descriptor.
 * @param idle The idle time before keepalive probes are sent.
 * @param count The number of keepalive probes to send before considering the connection dead.
 */
void syncs_set_keepalive(int socketfd, int idle, int count);

/**
 * @brief Sets the receive timeout for the specified socket.
 *
 * @param socketfd The socket file descriptor.
 * @param s The timeout seconds.
 * @param us The timeout microseconds.
 */
void syncs_set_rxtimeout(int socketfd, int s, int us);

/**
 * @brief Adds the specified socket to a multicast group.
 *
 * @param socketfd The socket file descriptor.
 * @param maddr The multicast address.
 * @param addr The local address.
 * @return 0 on success, -1 on failure.
 */
int syncs_add_multicast_group(int socketfd, const char *maddr, const char *addr);

/**
 * @brief Configures a multicast group address.
 *
 * @param gaddr The group socket address structure to be filled.
 * @param maddr The multicast address.
 * @param port The multicast port.
 */
void syncs_set_multicast_group(struct sockaddr_in *gaddr, const char *maddr, int port);

/**
 * @brief Adds a multicast route for the specified network interface.
 *
 * @param ifname The network interface name.
 * @return 0 on success, -1 on failure.
 */
int syncs_add_multicast_route(char *ifname);


#ifdef __cplusplus
}
#endif

#endif //__SYNCS_NET_H__

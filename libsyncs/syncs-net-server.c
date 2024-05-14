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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define MODULE_NAME "syncs-server"
#include <syncs-debug.h>
#undef syncsd_debug
#define syncsd_debug(fmt,args...)

int syncs_tcpserver_open(const char *addr, int port)
{
	struct sockaddr_in serveraddr;
	int socketfd;
	int reuse = 1;
	int nodelay = 1;
	int retry = 3;
	int ip_prio = IPTOS_LOWDELAY;
	struct linger linger = {1, 0};

	if ((socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		syncsd_error("can't open tcp socket:%s", strerror(errno));
		return -1;
	}

#ifdef SO_REUSEADDR
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(reuse)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
#endif
#ifdef SO_REUSEPORT
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, (const char*) &reuse, sizeof(reuse)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
#endif
#ifdef TCP_NODELAY
	if (setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (const char*) &nodelay, sizeof(nodelay)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
#endif
#ifdef TCP_QUICKACK
	if (setsockopt(socketfd, IPPROTO_TCP, TCP_QUICKACK, (const char*) &nodelay, sizeof(nodelay)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
#endif
	if (setsockopt(socketfd, IPPROTO_TCP, TCP_SYNCNT, (const char*) &retry, sizeof(retry)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
	if (setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (const char *) &linger, sizeof(linger)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}
	if (setsockopt(socketfd, IPPROTO_IP, IP_TOS, (const char*) &ip_prio, sizeof(ip_prio)) < 0) {
		syncsd_error("can't setting tcp socket:%s", strerror(errno));
	}

	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	if ((addr != NULL) && (*addr != 0)) serveraddr.sin_addr.s_addr = inet_addr(addr);
	else serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socketfd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in)) != 0) {
		close(socketfd);
		syncsd_error("can't bind tcp socket:%s", strerror(errno));
		return -2;
	}

	if (listen(socketfd, 5) != 0) {
		close(socketfd);
		syncsd_error("can't listen tcp socket:%s", strerror(errno));
		return -3;
	}

	return socketfd;
}

void syncs_tcpserver_close(int socketfd)
{
	shutdown(socketfd, 2);
}

int syncs_udpserver_open(const char *addr, int port)
{
	struct sockaddr_in serveraddr;
	int socketfd;
	int ip_prio = IPTOS_LOWDELAY;
	int reuse = 1;

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		syncsd_error("can't open udp socket:%s", strerror(errno));
		return -1;
	}

	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	if ((addr != NULL) && (*addr != 0)) serveraddr.sin_addr.s_addr = inet_addr(addr);
	else serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef SO_REUSEPORT
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, (const char*) &reuse, sizeof(reuse)) < 0) {
		syncsd_error("can't setting udp socket:%s", strerror(errno));
	}
#endif
	if (setsockopt(socketfd, IPPROTO_IP, IP_TOS, (const char*) &ip_prio, sizeof(ip_prio)) < 0) {
		syncsd_error("can't setting udp socket:%s", strerror(errno));
	}

	if (bind(socketfd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in)) != 0) {
		close(socketfd);
		syncsd_error("can't bind udp socket:%s", strerror(errno));
		return -2;
	}

	return socketfd;
}

void syncs_udpserver_close(int socketfd)
{
	close(socketfd);
}


int syncs_udpmulticast_open(const char *addr, int port)
{
	int socketfd;
	int reuse = 1;
	int loop = 1;
	struct sockaddr_in saddr;

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syncsd_error("can't open multucast socket:%s", strerror(errno));
		return -1;
	}

#ifdef SO_REUSEPORT
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, (const char*) &reuse, sizeof(reuse)) < 0) {
		syncsd_error("can't setting multucast socket:%s", strerror(errno));
	}
#endif
#ifdef IP_MULTICAST_LOOP
	if (setsockopt(socketfd, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*) &loop, sizeof(loop)) < 0) {
		syncsd_error("can't setting multicast socket:%s", strerror(errno));
	}
#endif

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	if ((addr != NULL) && (*addr != 0)) saddr.sin_addr.s_addr = inet_addr(addr);
	else saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in)) != 0) {
		close(socketfd);
		syncsd_error("can't bind udp socket:%s", strerror(errno));
		return -2;
	}

	return socketfd;
}

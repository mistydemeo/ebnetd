/*
 * Copyright (c) 1997, 98, 2000, 01  
 *    Motoyuki Kasahara
 *
 * Copyright (c) 2001
 *    IPv6 support by UMENO Takashi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_TYPE_SA_FAMILY_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, unistd.h)
 *   AC_CHECK_FUNCS(memcpy)
 *   AC_HEADER_TIME
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else /* not TIME_WITH_SYS_TIME */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* not HAVE_SYS_TIME_H */
#include <time.h>
#endif /* not HAVE_SYS_TIME_H */
#endif /* not TIME_WITH_SYS_TIME */

#include "dummyin6.h"

#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#include "getaddrinfo.h"
#endif

#include "serverport.h"
#include "fdset.h"

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif /* not HAVE_MEMCPY */

#undef LISTEN_NONBLOCK_SOCKETS

/*
 * Target address families.
 */
#ifdef ENABLE_IPV6
static sa_family_t target_families[] = {AF_INET6, AF_INET};
#define TARGET_FAMILY_COUNT 2
#else
static sa_family_t target_families[] = {AF_INET};
#define TARGET_FAMILY_COUNT 1
#endif

/*
 * Bind and listen to a TCP port.
 *
 * `port' is a port number to which the server listens.
 * `backlog' is a maximum queue length of pending connections.
 *
 * If successful, the file descriptor of the socket is returned.
 * Otherwise, -1 is returned.
 */

int
set_server_ports(port, backlog, listening_files, max_file)
    int port;
    int backlog;
    fd_set *listening_files;
    int *max_file;
{
    struct addrinfo hints;
    struct addrinfo *info_list;
    struct addrinfo *info;
    int socket_options;
    char address_string[INET6_ADDRSTRLEN];
    char port_string[16];
    int gai_error_code;
    int file;
    int i;

    initialize_fdset(listening_files, max_file);

    /*
     * Get address information.
     */
    sprintf(port_string, "%d", (int)port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    gai_error_code = getaddrinfo(NULL, port_string, &hints, &info_list);
    if (gai_error_code != 0) {
	syslog(LOG_DEBUG, "debug: getaddrinfo fail, %s",
	    gai_strerror(gai_error_code));
	goto failed;
    }

    for (i = 0; i < TARGET_FAMILY_COUNT; i++) {
	for (info = info_list; info != NULL; info = info->ai_next) {
	    if (info->ai_family != target_families[i])
		continue;

	    info->ai_addr->sa_family = info->ai_family;  /* for AIX. */

	    gai_error_code = getnameinfo(info->ai_addr, info->ai_addrlen, 
		address_string, sizeof(address_string), NULL, 0, 
		NI_NUMERICHOST | NI_NUMERICSERV);
	    if (gai_error_code != 0)
		continue;

	    /*
	     * Create a socket.
	     */
	    file = socket(info->ai_family, info->ai_socktype,
		info->ai_protocol);
	    if (file < 0) 
		continue;
#ifdef SO_REUSEADDR
	    socket_options = 1;
	    if (setsockopt(file, SOL_SOCKET, SO_REUSEADDR,
		(char *)&socket_options, sizeof(socket_options)) < 0) {
		syslog(LOG_ERR, "setsockopt() failed, %s: %s", strerror(errno),
		    address_string);
		goto failed;
	    }
#endif

	    /*
	     * Bind the socket.
	     */
	    if (bind(file, info->ai_addr, info->ai_addrlen) < 0) {
		syslog(LOG_ERR, "bind() failed, %s: %s", strerror(errno),
		    address_string);
		close(file);
		continue;
	    }

	    /*
	     * Listen to the port.
	     */
	    if (listen(file, backlog) < 0) {
		syslog(LOG_ERR, "listen() failed, %s: %s", strerror(errno),
		    address_string);
		goto failed;
	    }

	    add_fd_to_fdset(listening_files, max_file, file);

#ifdef LISTEN_NONBLOCK_SOCKETS
	    fcntl(file, F_GETFL, 0);
	    fcntl(file, F_SETFL, O_NONBLOCK);
#endif

	    syslog(LOG_DEBUG, "debug: set server port: %s", address_string);
	}
    }
    freeaddrinfo(info_list);

    if (*max_file < 0)
	syslog(LOG_ERR, "no server port available: %d/tcp", (int)port);

    return *max_file;

    /*
     * An error occurs...
     */
  failed:
    freeaddrinfo(info_list);
    syslog(LOG_ERR, "failed to listen to the port: %d/tcp", (int)port);
    return -1;
}


/*
 * Wait until a new connection comes.
 *
 * `file' is a socket to wait a connection.
 * The system call listen() must have been called for `file'.
 * (A file created by set_server_port() is suitable for `file'.)
 *
 * The function never returns until a new connection comes, or an error
 * occurs.  If an error occurs, -1 is returned.  Otherwise 0 is returned.
 */
int
wait_server_ports(listening_files, select_files, max_file)
    fd_set *listening_files;
    fd_set *select_files;
    int *max_file;
{
    int status;

    memcpy(select_files, listening_files, sizeof(fd_set));

    for (;;) {
	/*
	 * Wait until a new connection comes.
	 */
	status = select(*max_file + 1, select_files, NULL, NULL, NULL);
	if (0 < status)
	    break;
	else if (errno == EINTR) {
	    /*
	     * When a child process exits, select() returns -1 and `errno'
	     * is set to EINTR.
	     */ 
	    return -1;
	} else {
	    syslog(LOG_ERR, "select() failed, %s", strerror(errno));
	    goto failed;
	}
    }

    syslog(LOG_DEBUG, "debug: catch a new connection");
    return 0;

    /*
     * An error occurs...
     */
  failed:
    syslog(LOG_ERR, "failed to catch a new connection");
    return -1;
}


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
 *   AX_TYPE_SOCKLEN_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, unistd.h)
 *   AC_CHECK_FUNCS(strchr, memcpy)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "dummyin6.h"

#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#include "getaddrinfo.h"
#endif

#include "hostname.h"

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

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

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define isupper(c) ('A' <= (c) && (c) <= 'Z')
#define islower(c) ('a' <= (c) && (c) <= 'z')
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isupper(c) || islower(c) || isdigit(c))
#define isxdigit(c) \
 (isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define toupper(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define tolower(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))

#ifndef NI_WITHSCOPEID
#define NI_WITHSCOPEID	0
#endif

/*
 * Get remote host name and address of the socket `fd'.
 *
 * The host name and address are put into `host_name' and `address'.
 * If unknown, UNKNOWN_HOST is put into it.
 * If succeed, 0 is returned.  Otherwise -1 is returned.
 *
 */

#define FAMILY(addr) \
	(((struct sockaddr *)(addr))->sa_family)
#define IN6_ADDR(addr) \
	(&((struct sockaddr_in6 *)(addr))->sin6_addr)
#define IN_ADDR(addr) \
	(&((struct sockaddr_in *)(addr))->sin_addr)

int
identify_remote_host(file, host_name, address_string)
    int file;
    char *host_name;
    char *address_string;
{
    struct addrinfo hints;
    struct addrinfo *info_list;
    struct addrinfo *info;
    struct sockaddr_storage peer_name;
    socklen_t peer_name_length;
    int gai_error_code;
    char *p;

    /*
     * Set host name and address to UNKNOWN_HOST.
     */  
    strncpy(host_name, UNKNOWN_HOST, MAX_HOST_NAME_LENGTH);
    *(host_name + MAX_HOST_NAME_LENGTH) = '\0';
    strncpy(address_string, UNKNOWN_HOST, MAX_HOST_NAME_LENGTH);
    *(address_string + MAX_HOST_NAME_LENGTH) = '\0';

    /*
     * Get an IP address of the remote host by getpeername().
     */
    peer_name_length = sizeof(struct sockaddr_storage);

    if (getpeername(file, (struct sockaddr *)&peer_name, &peer_name_length)
	< 0) {
	syslog(LOG_ERR, "getpeername() failed, %s", strerror(errno));
	return -1;
    }

    if (FAMILY(&peer_name) != AF_INET6 && FAMILY(&peer_name) != AF_INET) {
	syslog(LOG_DEBUG, "peer host uses unknown protocol family");
	return -1;
    }

    gai_error_code = getnameinfo((struct sockaddr *)&peer_name,
	peer_name_length, address_string, MAX_HOST_NAME_LENGTH + 1, NULL, 0,
	NI_NUMERICHOST | NI_NUMERICSERV | NI_WITHSCOPEID);
    if (gai_error_code != 0) {
	syslog(LOG_DEBUG, "getnameinfo() failed, %s",
	    gai_strerror(gai_error_code));
	return -1;
    }

    /*
     * Get a host name of the remote host by getnameinfo().
     */
    gai_error_code = getnameinfo((struct sockaddr *)&peer_name,
	peer_name_length, host_name, MAX_HOST_NAME_LENGTH + 1, NULL, 0, 
	NI_NAMEREQD);
    if (gai_error_code != 0)
	return 0;
    
    /*
     * Verify the host name by reversed lookup.
     */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    gai_error_code = getaddrinfo(host_name, NULL, &hints, &info_list);
    if (gai_error_code != 0) {
	syslog(LOG_ERR, "getaddrinfo failed (reversed lookup), %s: %s",
	    gai_strerror(gai_error_code), host_name);
	return 0;
    }

    if (FAMILY(&peer_name) == AF_INET6
	&& IN6_IS_ADDR_V4MAPPED(IN6_ADDR(&peer_name))) {
	struct sockaddr_in v4_peer_name;
	
	*((unsigned char *)&v4_peer_name.sin_addr.s_addr)
	    = *((unsigned char *)IN6_ADDR(&peer_name)->s6_addr + 12);
	*((unsigned char *)&v4_peer_name.sin_addr.s_addr + 1)
	    = *((unsigned char *)IN6_ADDR(&peer_name)->s6_addr + 13);
	*((unsigned char *)&v4_peer_name.sin_addr.s_addr + 2)
	    = *((unsigned char *)IN6_ADDR(&peer_name)->s6_addr + 14);
	*((unsigned char *)&v4_peer_name.sin_addr.s_addr + 3)
	    = *((unsigned char *)IN6_ADDR(&peer_name)->s6_addr + 15);

	for (info = info_list; info; info = info->ai_next) {
	    if (info->ai_family == AF_INET6) {
		if (memcmp(IN6_ADDR(&peer_name), IN6_ADDR(info->ai_addr),
		    sizeof(struct in6_addr)) == 0) {
		    break;
		}
	    } else if (info->ai_family == AF_INET) {
		if (memcmp(IN_ADDR(&v4_peer_name), IN_ADDR(info->ai_addr),
		    sizeof(struct in_addr)) == 0) {
		    break;
		}
	    }
	}
    } else if (FAMILY(&peer_name) == AF_INET6) {
	for (info = info_list; info; info = info->ai_next) {
	    if (info->ai_family == AF_INET6) {
		if (memcmp(IN6_ADDR(&peer_name), IN6_ADDR(info->ai_addr), 
		    sizeof(struct in6_addr)) == 0) {
		    break;
		}
	    }
	}
    } else {
	/* FAMILY(&peer_name) == AF_INET */
	for (info = info_list; info; info = info->ai_next) {
	    if (info->ai_family == AF_INET) {
		if (memcmp(IN_ADDR(&peer_name), IN_ADDR(info->ai_addr), 
		    sizeof(struct in_addr)) == 0) {
		    break;
		}
	    }
	}
    }

    if (info == NULL) {
	syslog(LOG_ERR, "reversed lookup mismatch: %s -> %s",
	    address_string, host_name);
	strncpy(host_name, UNKNOWN_HOST, MAX_HOST_NAME_LENGTH);
	*(host_name + MAX_HOST_NAME_LENGTH) = '\0';
	return 0;
    }

    /*
     * Change the host name to lower cases.
     */
    for (p = host_name; *p != '\0'; p++) {
	if (isupper(*p))
	    *p = tolower(*p);
    }

    freeaddrinfo(info_list);
    return 0;
}

/*
 * Copyright (c) 2002
 *    Motoyuki Kasahara
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
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, stdlib.h)
 *   AC_CHECK_FUNCS(inet_pton strchr memcpy)
 *
 *   and HAVE_STRUCT_IN6_ADDR check.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifndef VOID
#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif
#endif

#include "inet_pton2.h"

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

#ifndef IF_NAMESIZE
#ifdef IFNAMSIZ 
#define IF_NAMESIZE             IFNAMSIZ
#else 
#define IF_NAMESIZE             16
#endif
#endif

#ifdef __STDC__
static int inet_pton2_ipv6(const char *, struct in6_addr *, char *);
static int inet_pton2_ipv4(const char *, struct in_addr *, char *);
static int inet_prefix_pton_ipv6(const char *, struct in6_addr *,
    struct in6_addr *, char *);
static void ipv6_net_mask_length_to_address(int, struct in6_addr *);
static int inet_prefix_pton_ipv4(const char *, struct in_addr *,
    struct in_addr *, char *);
static void ipv4_net_mask_length_to_address(int, struct in_addr *);
#else
static int inet_pton2_ipv6();
static int inet_pton2_ipv4();
static int inet_prefix_pton_ipv6();
static void ipv6_net_mask_length_to_address();
static int inet_prefix_pton_ipv4();
static void ipv4_net_mask_length_to_address();
#endif

/*
 * inet_pton() with scope name extension.
 *
 * inet_pton2() has the extra argument `interface'.
 * If the given address has a scope name, the scope name is put into
 * `interface' upon return.  The maximum length of a scope name
 * is IF_NAMESIZE.
 *
 * Note that only IPv6 address can have a scope name.
 */
int
inet_pton2(family, string, address, interface)
    int family;
    const char *string;
    VOID *address;
    char *interface;
{
    if (family == AF_INET6)
	return inet_pton2_ipv6(string, address, interface);
    else if (family == AF_INET)
	return inet_pton2_ipv4(string, address, interface);

#ifdef EAFNOSUPPORT
    errno = EAFNOSUPPORT;
#else
    errno = EINVAL;
#endif
    return -1;
}


/*
 * Internal function of inet_pton2().
 * This function parses IPv6 address (e.g. "ff02::%fxp").
 */
int
inet_pton2_ipv6(string, address, interface)
    const char *string;
    struct in6_addr *address;
    char *interface;
{
    struct in6_addr address_buffer;
    char *copied_string = NULL;
    char *percent;
    int result;
    int saved_errno;

    /*
     * Copy `string' to `copied_string'.
     */
    copied_string = (char *)malloc(strlen(string) + 1);
    if (copied_string == NULL) {
	result = -1;
	goto failed;
    }
    strcpy(copied_string, string);

    /*
     * Get scope name.
     */
    percent = strchr(copied_string, '%');
    if (percent != NULL) {
	if (*(percent + 1) == '\0' || IF_NAMESIZE < strlen(percent + 1) + 1) {
	    result = 0;
	    goto failed;
	}
	*percent = '\0';
    }

    /*
     * Parse address.
     */
    result = inet_pton(AF_INET6, copied_string, &address_buffer);
    if (result != 1)
	goto failed;

    /*
     * Set `address', `net_mask' and `interface'.
     */
    memcpy(address, &address_buffer, sizeof(struct in6_addr));
    if (interface != NULL)
	strcpy(interface, "");

    free(copied_string);
    return 1;

    /*
     * An error occurs...
     */
  failed:
    saved_errno = errno;
    if (copied_string != NULL)
	free(copied_string);
    errno = saved_errno;
    return result;
}


/*
 * Internal function of inet_prefix_pton2().
 * This function parses IPv4 address (e.g. "192.16.0").
 */
int
inet_pton2_ipv4(string, address, interface)
    const char *string;
    struct in_addr *address;
    char *interface;
{
    struct in_addr address_buffer;
    int result;

    /*
     * Parse address.
     */
    result = inet_pton(AF_INET, string, &address_buffer);
    if (result != 1)
	return result;

    /*
     * Set `address', `net_mask' and `interface'.
     */
    memcpy(address, &address_buffer, sizeof(struct in_addr));
    if (interface != NULL)
	strcpy(interface, "");

    return 1;
}


/*
 * Converts a presentation format IP-prefix to network format.
 * IP-prefix is a value with one of the following forms:
 *
 *     <ip-address>/<net_mask_length>
 *     <ip-address>
 *
 * For example, `ff02::/96' is a valid IPv6-prefix, and `192.168.0/24'
 * is a vald IPv4-prefix.  Also IPv4 and IPv6 addresses are valid
 * IP-prefixes.
 *
 * The meaning of return value is the same as that of inet_pton().
 * It returns 1 if the address was valid for the specified address
 * family, or 0 if the address wasn't parseable in the specified
 * address family, or -1 if some system error occurred (in which case
 * `errno' will have been set).
 */
int
inet_prefix_pton(family, string, address, net_mask)
    int family;
    const char *string;
    VOID *address;
    VOID *net_mask;
{
    if (family == AF_INET6)
	return inet_prefix_pton_ipv6(string, address, net_mask, NULL);
    else if (family == AF_INET)
	return inet_prefix_pton_ipv4(string, address, net_mask, NULL);

#ifdef EAFNOSUPPORT
    errno = EAFNOSUPPORT;
#else
    errno = EINVAL;
#endif
    return -1;
}

/*
 * inet_prefix_pton() with scope name extension.
 * For details about the function, See inet_pton2() and inet_prefix_pton().
 */
int
inet_prefix_pton2(family, string, address, net_mask, interface)
    int family;
    const char *string;
    VOID *address;
    VOID *net_mask;
    char *interface;
{
    if (family == AF_INET6)
	return inet_prefix_pton_ipv6(string, address, net_mask, interface);
    else if (family == AF_INET)
	return inet_prefix_pton_ipv4(string, address, net_mask, interface);

#ifdef EAFNOSUPPORT
    errno = EAFNOSUPPORT;
#else
    errno = EINVAL;
#endif
    return -1;
}

/*
 * Internal function of inet_prefix_pton2().
 * This function parses IPv6 prefix (e.g. "ff02::%fxp0/96").
 */
static int
inet_prefix_pton_ipv6(string, address, net_mask, interface)
    const char *string;
    struct in6_addr *address;
    struct in6_addr *net_mask;
    char *interface;
{
    char *copied_string = NULL;
    struct in6_addr address_buffer;
    struct in6_addr net_mask_buffer;
    unsigned char mask_buffer[16];
    char *percent;
    char *slash;
    int net_mask_length;
    int result;
    int saved_errno;
    int i;

    /*
     * Copy `string' to `copied_string'.
     */
    copied_string = (char *)malloc(strlen(string) + 1);
    if (copied_string == NULL) {
	result = -1;
	goto failed;
    }
    strcpy(copied_string, string);

    /*
     * Get net mask length.
     */
    slash = strchr(copied_string, '/');
    if (slash != NULL) {
	char *p = slash + 1;

	if (*p == '0') {
	    p++;
	} else if ('1' <= *p && *p <= '9') {
	    p++;
	    while ('0' <= *p && *p <= '9')
		p++;
	}

	if (*p != '\0' || p == slash + 1 || slash + 4 < p) {
	    result = 0;
	    goto failed;
	}
	*slash = '\0';

	net_mask_length = atoi(slash + 1);
	if (128 < net_mask_length) {
	    result = 0;
	    goto failed;
	}
    } else {
	net_mask_length = 128;
    }
    ipv6_net_mask_length_to_address(net_mask_length, &net_mask_buffer);

    /*
     * Get scope name.
     */
    percent = strchr(copied_string, '%');
    if (percent != NULL) {
	if (*(percent + 1) == '\0'
	    || IF_NAMESIZE < strlen(percent + 1) + 1) {
	    result = 0;
	    goto failed;
	}
	*percent = '\0';
    }

    /*
     * Parse address.
     */
    result = inet_pton(AF_INET6, copied_string, &address_buffer);
    if (result != 1)
	goto failed;

    /*
     * Check address/netmask pair.
     */
    for (i = 0; i < 16; i++) {
	mask_buffer[i]
	    = *((unsigned char *)address_buffer.s6_addr + i)
	    & *((unsigned char *)net_mask_buffer.s6_addr + i);
    }
    if (memcmp(mask_buffer, &address_buffer.s6_addr, 16) != 0) {
	result = 0;
	goto failed;
    }

    /*
     * Set `address', `net_mask' and `interface'.
     */
    memcpy(address, &address_buffer, sizeof(struct in6_addr));
    memcpy(net_mask, &net_mask_buffer, sizeof(struct in6_addr));
    if (interface != NULL) {
	if (percent != NULL)
	    strcpy(interface, percent + 1);
	else
	    strcpy(interface, "");
    }

    free(copied_string);
    return 1;

    /*
     * An error occurs...
     */
  failed:
    saved_errno = errno;
    if (copied_string != NULL)
	free(copied_string);
    errno = saved_errno;
    return result;
}


/*
 * Convert IPv6 netmask length to IPv6 address.
 * For example, 60 is converted to FFFF:FFFF:FFFF:FFF0::.
 */
static void
ipv6_net_mask_length_to_address(length, address)
    int length;
    struct in6_addr *address;
{
    static const unsigned char mod8_masks[]
	= {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    unsigned char *address_p = (unsigned char *)address->s6_addr;
    int rest_length = length;

    memset(address->s6_addr, 0, 16);

    while (8 <= rest_length) {
	*address_p++ = 0xff;
	rest_length -= 8;
    }

    if (0 < rest_length)
	*address_p = mod8_masks[rest_length];
}


/*
 * Internal function of inet_prefix_pton2().
 * This function parses IPv4 prefix (e.g. "192.16.0/24").
 */
static int
inet_prefix_pton_ipv4(string, address, net_mask, interface)
    const char *string;
    struct in_addr *address;
    struct in_addr *net_mask;
    char *interface;
{
    char *copied_string = NULL;
    struct in_addr address_buffer;
    struct in_addr net_mask_buffer;
    char *slash;
    int net_mask_length;
    int result;
    int saved_errno;

    /*
     * Copy `string' to `copied_string'.
     */
    copied_string = (char *)malloc(strlen(string) + 1);
    if (copied_string == NULL) {
	result = -1;
	goto failed;
    }
    strcpy(copied_string, string);

    /*
     * Get net mask length.
     */
    slash = strchr(copied_string, '/');
    if (slash != NULL) {
	char *p = slash + 1;

	if (*p == '0') {
	    p++;
	} else if ('1' <= *p && *p <= '9') {
	    p++;
	    while ('0' <= *p && *p <= '9')
		p++;
	}

	if (*p != '\0' || p == slash + 1 || slash + 3 < p) {
	    result = 0;
	    goto failed;
	}
	*slash = '\0';

	net_mask_length = atoi(slash + 1);
	if (32 < net_mask_length) {
	    result = 0;
	    goto failed;
	}
    } else {
	net_mask_length = 32;
    }
    ipv4_net_mask_length_to_address(net_mask_length, &net_mask_buffer);

    /*
     * Parse address.
     */
    result = inet_pton(AF_INET, copied_string, &address_buffer);
    if (result != 1)
	goto failed;

    /*
     * Check address/netmask pair.
     */
    if ((address_buffer.s_addr & net_mask_buffer.s_addr)
	!= address_buffer.s_addr) {
	result = 0;
	goto failed;
    }

    /*
     * Set `address', `net_mask' and `interface'.
     */
    memcpy(address, &address_buffer, sizeof(struct in_addr));
    memcpy(net_mask, &net_mask_buffer, sizeof(struct in_addr));
    if (interface != NULL)
	strcpy(interface, "");

    free(copied_string);
    return 1;

    /*
     * An error occurs...
     */
  failed:
    saved_errno = errno;
    if (copied_string != NULL)
	free(copied_string);
    errno = saved_errno;
    return result;
}


/*
 * Convert IPv4 netmask length to IPv4 address.
 * For example, 26 is converted to 255.255.255.192.
 */
static void
ipv4_net_mask_length_to_address(length, address)
    int length;
    struct in_addr *address;
{
    static const unsigned char mod8_masks[]
	= {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    unsigned char *address_p = (unsigned char *)&address->s_addr;
    int rest_length = length;

    memset(&address->s_addr, 0, 4);

    while (8 <= rest_length) {
	*address_p++ = 0xff;
	rest_length -= 8;
    }

    if (0 < rest_length)
	*address_p = mod8_masks[rest_length];
}


/*
 * Test program.
 */
#ifdef TEST
int
main()
{
    char line[512];
    char *newline;
    struct in6_addr ipv6_address, ipv6_net_mask;
    struct in_addr ipv4_address, ipv4_net_mask;
    char interface[32];

    while (fgets(line, sizeof(line), stdin) != NULL) {
	newline = strchr(line, '\n');
	if (newline != NULL)
	    *newline = '\0';
	if (inet_prefix_pton2(AF_INET6, line, &ipv6_address, &ipv6_net_mask,
	    interface) == 1) {
	    printf(">> IPv6 prefix\n");
	}
	if (inet_prefix_pton2(AF_INET, line, &ipv4_address, &ipv4_net_mask,
	    interface) == 1) {
	    printf(">> IPv4 prefix\n");
	}
	if (inet_pton2(AF_INET6, line, &ipv6_address, interface) == 1) {
	    printf(">> IPv6 address\n");
	}
	if (inet_pton2(AF_INET, line, &ipv4_address, interface) == 1) {
	    printf(">> IPv4 address\n");
	}
    }

    return 0;
}

#endif /* TEST */

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
 *   AC_CHECK_HEADERS(string.h, memory.h)
 *   AC_CHECK_FUNCS(inet_pton memcpy)
 *
 *   and HAVE_STRUCT_IN6_ADDR check.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netinet/in.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

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
 * Prefix of IPv4 mapped IPv6 address.
 */
static const unsigned char mapped_ipv6_address_prefix[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff
};
    
static const unsigned char mapped_ipv6_net_mask_prefix[] = {
    0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff
};
    
void
ipv4_to_mapped_ipv6_address(ipv6_address, ipv4_address)
    struct in6_addr *ipv6_address;
    const struct in_addr *ipv4_address;
{
    memcpy(&ipv6_address->s6_addr, mapped_ipv6_address_prefix, 12);
    memcpy((unsigned char *)ipv6_address->s6_addr + 12,
	&ipv4_address->s_addr, 4);
}

int
mapped_ipv6_to_ipv4_address(ipv4_address, ipv6_address)
    struct in_addr *ipv4_address;
    const struct in6_addr *ipv6_address;
{
    if (memcmp(&ipv6_address->s6_addr, mapped_ipv6_address_prefix, 12) != 0)
	return -1;

    memcpy(&ipv4_address->s_addr,
	(unsigned char *)ipv6_address->s6_addr + 12, 4);
    return 0;
}


void
ipv4_to_mapped_ipv6_net_mask(ipv6_net_mask, ipv4_net_mask)
    struct in6_addr *ipv6_net_mask;
    const struct in_addr *ipv4_net_mask;
{
    memcpy(&ipv6_net_mask->s6_addr, mapped_ipv6_net_mask_prefix, 12);
    memcpy((unsigned char *)ipv6_net_mask->s6_addr + 12,
	&ipv4_net_mask->s_addr, 4);
}

int
mapped_ipv6_to_ipv4_net_mask(ipv4_net_mask, ipv6_net_mask)
    struct in_addr *ipv4_net_mask;
    const struct in6_addr *ipv6_net_mask;
{
    if (memcmp(&ipv6_net_mask->s6_addr, mapped_ipv6_net_mask_prefix, 12) != 0)
	return -1;

    memcpy(&ipv4_net_mask->s_addr,
	(unsigned char *)ipv6_net_mask->s6_addr + 12, 4);
    return 0;
}


/*
 * Test program.
 */
#ifdef TEST
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int
main()
{
    char line[512];
    char result[512];
    char *newline;
    struct in6_addr ipv6_address;
    struct in_addr ipv4_address;

    while (fgets(line, sizeof(line), stdin) != NULL) {
	newline = strchr(line, '\n');
	if (newline != NULL)
	    *newline = '\0';
	if (inet_pton(AF_INET6, line, &ipv6_address) == 1) {
	    if (mapped_ipv6_to_ipv4_address(&ipv4_address, &ipv6_address)
		< 0) {
		printf(">> IPv6 address, but not IPv4 mapped\n");
		continue;
	    }
	    if (inet_ntop(AF_INET, &ipv4_address, result, sizeof(result))
		== NULL) {
		printf(">> internal error?, inet_ntop() failed\n");
		continue;
	    }
	} else if (inet_pton(AF_INET, line, &ipv4_address) == 1) {
	    ipv4_to_mapped_ipv6_address(&ipv6_address, &ipv4_address);
	    if (inet_ntop(AF_INET6, &ipv6_address, result, sizeof(result))
		== NULL) {
		printf(">> internal error?, inet_ntop() failed\n");
		continue;
	    }
	} else {
	    printf(">> not IP address\n");
	    continue;
	}

	printf(">> %s\n", result);
    }

    return 0;
}

#endif /* TEST */

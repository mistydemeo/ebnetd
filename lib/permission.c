/*
 * Copyright (c) 1997, 98, 2000, 01  
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
 *   AC_CHECK_FUNCS(inet_pton memcpy strchr)
 *
 *   and HAVE_STRUCT_IN6_ADDR check.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <syslog.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "dummyin6.h"
#include "mappedaddr.h"
#include "inet_pton2.h"

#include "permission.h"

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

#ifndef HAVE_INET_PTON
#ifdef __STDC__
int inet_pton(int, const char *, void *);
#else
int inet_pton();
#endif
#endif

#ifdef __STDC__
static const char *strnopbrk(const char *, const char *);
#else
static const char *strnopbrk();
#endif


/*
 * Initialize `permission'.
 */
void
initialize_permission(permission)
    Permission *permission;
{
    permission->next = NULL;
}


/*
 * Free all nodes in `permission'.
 */
void
finalize_permission(permission)
    Permission *permission;
{
    Permission *permission_p;
    Permission *next;

    permission_p = permission->next;
    while (permission_p != NULL) {
	next = permission_p->next;
	if (permission_p->host_name != NULL)
	    free(permission_p->host_name);
	free(permission_p);
	permission_p = next;
    }

    permission->next = NULL;
}


/*
 * Add `pattern' to the list `permission'.
 *
 * If `permission' is a valid IPv4/IPv6 prefix or address, it is recognized
 * as it is.  Otherwise `pattern' is recognized as a host name.
 *
 * The function returns 0 upon successful, -1 otherwise.
 */
int
add_permission(permission, pattern)
    Permission *permission;
    const char *pattern;
{
    Permission *permission_p = NULL;
    const char *pattern_p = pattern;
    struct in_addr ipv4_address, ipv4_net_mask;

    /*
     * Allocate memories for a new node and a host name.
     */
    permission_p = (Permission *)malloc(sizeof(Permission));
    if (permission_p == NULL) {
	syslog(LOG_ERR, "memory exhausted");
	goto failed;
    }
    permission_p->not = 0;
    permission_p->host_name = NULL;
    memcpy(&permission_p->address, &in6addr_any, sizeof(struct in6_addr));
    memcpy(&permission_p->net_mask, &in6addr_any, sizeof(struct in6_addr));
    permission_p->scope_name[0] = '\0';
    permission_p->next = NULL;
 
    /*
     * Set <NOT> flag.
     */
    if (*pattern_p == '!') {
	permission_p->not = 1;
	pattern_p++;
    }

    /*
     * Parse the given pattern.
     */
    if (strchr(pattern_p, ':') != NULL) {
	/*
	 * The pattern seems to be an IPv6 address.
	 */
	if (inet_prefix_pton2(AF_INET6, pattern_p, &permission_p->address,
	    &permission_p->net_mask, permission_p->scope_name) != 1) {
	    syslog(LOG_ERR, "invalid IPv6 prefix: %s", pattern_p);
	    goto failed;
	}

    } else if (strnopbrk(pattern_p, "0123456789./") == NULL) {
	/*
	 * The pattern seems to be an IPv4 address.
	 */
	if (inet_prefix_pton(AF_INET, pattern_p, &ipv4_address,
	    &ipv4_net_mask) != 1) {
	    syslog(LOG_ERR, "invalid IPv4 prefix: %s", pattern_p);
	    goto failed;
	}
	ipv4_to_mapped_ipv6_address(&permission_p->address, &ipv4_address);
	ipv4_to_mapped_ipv6_net_mask(&permission_p->net_mask, &ipv4_net_mask);

    } else {
	/*
	 * The pattern seems to be a host name.
	 */
	permission_p->host_name = (char *)malloc(strlen(pattern_p) + 1);
	if (permission_p->host_name == NULL) {
	    syslog(LOG_ERR, "memory exhausted");
	    goto failed;
	}
	strcpy(permission_p->host_name, pattern_p);
    }

    /*
     * Insert the new node into the permission list.
     */
    permission_p->next = permission->next;
    permission->next = permission_p;

    return 0;

    /*
     * If an error occurs...
     */
  failed:
    if (permission_p != NULL) {
	if (permission_p->host_name == NULL)
	    free(permission_p->host_name);
	free(permission_p);
    }
    return -1;
}


/*
 * strnopbrk() is similar to strpbrk(), but it locates the string `s'
 * the first occurrence of any character *NOT* listed in `charset'.
 */
static const char *
strnopbrk(string, charset)
    const char *string;
    const char *charset;
{
    unsigned char table[256];	/* assumes CHAR_BITS is 8 */
    const unsigned char *string_p = (const unsigned char *)string;
    const unsigned char *charset_p = (const unsigned char *)charset;

    memset(table, 0, sizeof(table));

    while (*charset_p != '\0')
	table[*charset_p++]  = 1;

    while (*string_p != '\0') {
	if (!table[*string_p])
	    return (const char *)string_p;
	string_p++;
    }

    return NULL;
}


/*
 * Examine access permission to `host_name' and `address'.
 * If access is permitted, 1 is returned.  Otherwise 0 is returned.
 */
int
test_permission(permission, host_name, address_string, function)
    const Permission *permission;
    const char *host_name;
    const char *address_string;
#ifdef __STDC__
    int (*function)(const char *, const char *);
#else
    int (*function)();
#endif
{
    const Permission *permission_p;
    int result = 0;
    struct in6_addr ipv6_address;
    struct in_addr ipv4_address;
    char scope_name[IF_NAMESIZE];
    unsigned char mask_buffer[16];
    int i;

    if (inet_pton2(AF_INET6, address_string, &ipv6_address, scope_name) == 1) {
	/* nothing to do */
    } else if (inet_pton(AF_INET, address_string, &ipv4_address) == 1) {
	ipv4_to_mapped_ipv6_address(&ipv6_address, &ipv4_address);
	scope_name[0] = '\0';
    }

    /*
     * Test.
     */
    for (permission_p = permission->next; permission_p != NULL;
	 permission_p = permission_p->next) {
	if (permission_p->host_name != NULL) {
	    if (function(permission_p->host_name, host_name))
		result = permission_p->not ? 0 : 1;
	} else {
	    for (i = 0; i < 16; i++) {
		mask_buffer[i] = *((unsigned char *)ipv6_address.s6_addr + i)
		    & *((unsigned char *)permission_p->net_mask.s6_addr + i);
	    }
	    if (memcmp(mask_buffer, permission_p->address.s6_addr, 16) == 0) {
		if (permission_p->scope_name[0] == '\0'
		    || strcmp(permission_p->scope_name, scope_name) == 0) {
		    result = permission_p->not ? 0 : 1;
		}
	    }
	}
    }

    return result;
}


/*
 * Count the entires of the permission list, and return it.
 */
int
count_permission(permission)
    const Permission *permission;
{
    const Permission *permission_p;
    int count = 0;

    for (permission_p = permission->next; permission_p != NULL;
	 permission_p = permission_p->next)
	count++;

    return count;
}


/*
 * Test program.
 */
#ifdef TEST
int streq(s1, s2)
    const char *s1;
    const char *s2;
{
    return (strcmp(s1, s2) == 0);
}

int
main(argc, argv)
    int argc;
    char *argv[];
{
    Permission permission;
    FILE *file;
    char buffer[512];
    char *newline;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s access-list-file\n", argv[0]);
	return 1;
    }

    initialize_permission(&permission);
    set_fakelog_mode(FAKELOG_TO_STDERR);

    file = fopen(argv[1], "r");
    if (file == NULL) {
	fprintf(stderr, "%s: failed to open the file: %s\n", argv[0], argv[1]);
	return 1;
    }

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
	newline = strchr(buffer, '\n');
	if (newline != NULL)
	    *newline = '\0';
	if (add_permission(&permission, buffer) < 0) {
	    fprintf(stderr, "%s: failed to add permission to the list: %s\n",
		argv[0], buffer);
	    fclose(file);
	    return 1;
	}
    }
    fclose(file);

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
	newline = strchr(buffer, '\n');
	if (newline != NULL)
	    *newline = '\0';
	if (test_permission(&permission, "?", buffer, streq))
	    printf("allowed\n");
	else
	    printf("denied\n");
    }

    return 0;
}

#endif /* TEST */

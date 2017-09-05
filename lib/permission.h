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

#ifndef PERMISSION_H
#define PERMISSION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>

#include "dummyin6.h"

/*
 * Access permission list.
 */
typedef struct Permission_Struct Permission;
struct Permission_Struct {
    int not;				/* inverse permission */
    char *host_name;			/* host name pattern */
    struct in6_addr address;		/* address */
    struct in6_addr net_mask;		/* net mask */
    char scope_name[IF_NAMESIZE];	/* scope name pattern */
    Permission *next;			/* pointer to next node */
};



/*
 * Function declarations.
 */
#ifdef __STDC__
void initialize_permission(Permission *);
void finalize_permission(Permission *);
int add_permission(Permission *, const char *);
int test_permission(const Permission *, const char *, const char *,
    int (*func)(const char *, const char *));
int count_permission(const Permission *);

#else /* not __STDC__ */
void initialize_permission();
void finalize_permission();
int add_permission();
int test_permission();
int count_permission();
#endif /* not __STDC__ */

#endif /* not PERMISSION_H */

/*
 * Copyright (c) 1996, 2000, 01  
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

#ifndef HOSTNAME_H
#define HOSTNAME_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * The string represents unknown hostname or address.
 */
#ifndef UNKNOWN_HOST
#define UNKNOWN_HOST		"?"
#endif


/*
 * The maximum length of hostname and address.
 */
#ifdef ENABLE_IPV6
#define MAX_HOST_NAME_LENGTH	1023
#else
#define MAX_HOST_NAME_LENGTH	127
#endif


/*
 * Function declarations.
 */
#ifdef __STDC__
int identify_remote_host(int, char *, char *);
#else /* not __STDC__ */
int identify_remote_host();
#endif /* __STDC__ */

#endif /* not HOSTNAME_H */

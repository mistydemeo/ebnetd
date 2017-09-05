/*
 * Copyright (c) 1997, 2000, 01  
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

#ifndef SERVERPORT_H
#define SERVERPORT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * Function declarations.
 */
#ifdef __STDC__
int set_server_ports(int, int, fd_set *, int *);
int wait_server_ports(fd_set *, fd_set *, int *);
#else /* not __STDC__ */
int set_server_ports();
int wait_server_ports();
#endif /* not __STDC__ */

#endif /* not SERVERPORT_H */

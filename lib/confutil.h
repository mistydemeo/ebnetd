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

#ifndef CONFUTIL_H
#define CONFUTIL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/*
 * Function declarations.
 */
#ifdef __STDC__
int parse_integer(const char *, int *);
int parse_tcp_port(const char *, in_port_t *);
int parse_udp_port(const char *, in_port_t *);
int parse_user(const char *, uid_t *);
int parse_group(const char *, gid_t *);
int parse_on_off(const char *, int *);
int parse_syslog_facility(const char *, int *);
#else /* not __STDC__ */
int parse_integer();
int parse_tcp_port();
int parse_udp_port();
int parse_user();
int parse_group();
int parse_on_off();
int parse_syslog_facility();
#endif /* not __STDC__ */

#endif /* not CONFUTIL_H */

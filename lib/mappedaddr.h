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

#ifndef MAPPEDADDR_H
#define MAPPEDADDR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netinet/in.h>

#ifdef __STDC__
void ipv4_to_mapped_ipv6_address(struct in6_addr *, const struct in_addr *);
int mapped_ipv6_to_ipv4_address(struct in_addr *, const struct in6_addr *);
void ipv4_to_mapped_ipv6_net_mask(struct in6_addr *, const struct in_addr *);
int mapped_ipv6_to_ipv4_net_mask(struct in_addr *, const struct in6_addr *);
#else
void ipv4_to_mapped_ipv6_address();
int mapped_ipv6_to_ipv4_address();
void ipv4_to_mapped_ipv6_net_mask();
int mapped_ipv6_to_ipv4_net_mask();
#endif

#endif /* MAPPEDADDR_H */

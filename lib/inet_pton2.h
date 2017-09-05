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

#ifndef INET_PTON2_H
#define INET_PTON2_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifndef VOID
#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif
#endif

#ifdef __STDC__
int inet_pton2(int, const char *, VOID *, char *);
int inet_prefix_pton(int, const char *, VOID *, VOID *);
int inet_prefix_pton2(int, const char *, VOID *, VOID *, char *);
#else
int inet_pton2();
int inet_prefix_pton();
int inet_prefix_pton2();
#endif

#endif /* INET_PTON2_H */

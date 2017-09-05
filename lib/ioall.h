/*
 * Copyright (c) 1997, 2000, 01, 02
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

#ifndef IOALL_H
#define IOALL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

/*
 * Function declarations.
 */
#ifdef __STDC__
int read_all(int, int, void *, size_t);
int write_all(int, int, const void *, size_t);
int write_string_all(int, int, const char *);
#else /* not __STDC__ */
int read_all();
int write_all();
int write_string_all();
#endif /* not __STDC__ */

#endif /* not IOALL_H */

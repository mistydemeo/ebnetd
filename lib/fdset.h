/*
 * Copyright (c) 2001  Takashi UMENO
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

#ifndef FD_SET_H
#define FD_SET_H

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
void initialize_fdset(fd_set *, int *);
void finalize_fdset(fd_set *, int *);
void add_fd_to_fdset(fd_set *, int *, int);
int shutdown_fdset(fd_set *, int *, int);
int close_fdset(fd_set *, int *);
#else /* not __STDC__ */
void initialize_fdset();
void finalize_fdset();
void add_fd_to_fdset();
int shutdown_fdset();
int close_fdset();
#endif /* not __STDC__ */

#endif /* not FD_SET_H */

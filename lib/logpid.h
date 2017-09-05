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

#ifndef LOGPID_H
#define LOGPID_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>


/*
 * Function declarations.
 */
#ifdef __STDC__
int log_pid_file(const char *);
int remove_pid_file(const char *);
int probe_pid_file(const char *);
int read_pid_file(const char *, pid_t *);
#else
int log_pid_file();
int remove_pid_file();
int probe_pid_file();
int read_pid_file();
#endif

#endif /* not LOGPID_H */

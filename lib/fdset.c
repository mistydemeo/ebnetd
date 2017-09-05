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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * FreeBSD uses bzero() in FD_ZERO().
 */
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else /* not TIME_WITH_SYS_TIME */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* not HAVE_SYS_TIME_H */
#include <time.h>
#endif /* not HAVE_SYS_TIME_H */
#endif /* not TIME_WITH_SYS_TIME */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef SHUT_RD
#define SHUT_RD 0
#endif
#ifndef SHUT_WR
#define SHUT_WR 1
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#include "fdset.h"

/*
 * Initialize socket descriptors.
 */
void
initialize_fdset(fdset, max_fd)
    fd_set *fdset;
    int *max_fd;
{
    FD_ZERO(fdset);
    *max_fd = -1;
}

/*
 * Finalize socket descriptors.
 */
void
finalize_fdset(fdset, max_fd)
    fd_set *fdset;
    int *max_fd;
{
    shutdown_fdset(fdset, max_fd, SHUT_RDWR);
    close_fdset(fdset, max_fd);
}

/*
 * Add file to socket discritors.
 */
void
add_fd_to_fdset(fdset, max_fd, fd)
    fd_set *fdset;
    int *max_fd;
    int fd;
{
    FD_SET(fd, fdset);

    if (fd > *max_fd)
	*max_fd = fd;
}

/*
 * Shutdown multi sockets 
 */
int
shutdown_fdset(fdset, max_fd, how)
    fd_set *fdset;
    int *max_fd;
    int how;
{
    int i;
    
    for (i = 0; i <= *max_fd; i++) {
	if (FD_ISSET(i, fdset)) {
	    shutdown(i, how);
	    FD_CLR(i, fdset);
	}
    }
    return 0;
}

/*
 * Close multi sockets 
 */
int
close_fdset(fdset, max_fd)
    fd_set *fdset;
    int *max_fd;
{
    int i;

    for (i = 0; i <= *max_fd; i++) {
	if (FD_ISSET(i, fdset)) {
	    close(i);
	    FD_CLR(i, fdset);
	}
    }
    *max_fd = -1;
    return 0;
}



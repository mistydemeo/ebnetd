/*
 * Copyright (c) 1997, 98, 99, 2000, 01, 02
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

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_CHECK_TYPE(ssize_t, int)
 *   AC_CHECK_HEADERS(unistd.h, fcntl.h)
 *   AC_HEADER_TIME
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <errno.h>
#include <syslog.h>

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifndef VOID
#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif
#endif

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

/*
 * Read data from a file.
 * It repeats to call read() until all data will have been read.
 * The function returns 1 upon success, 0 upon timeout, -1 upon error. 
 */
int
read_all(file, timeout, buffer, length)
    int file;
    int timeout;
    VOID *buffer;
    size_t length;
{
    char *buffer_p = buffer;
    ssize_t rest_length = length;
    fd_set fdset;
    struct timeval timeval;
    int select_result;
    ssize_t read_result;

    while (0 < rest_length) {
	errno = 0;
	FD_ZERO(&fdset);
	FD_SET(file, &fdset);

	if (timeout == 0)
	    select_result = select(file + 1, &fdset, NULL, NULL, NULL);
	else {
	    timeval.tv_sec = timeout;
	    timeval.tv_usec = 0;
	    select_result = select(file + 1, &fdset, NULL, NULL, &timeval);
	}
	if (select_result < 0) {
	    if (errno == EINTR)
		continue;
	    syslog(LOG_INFO, "failed to select, %s", strerror(errno));
	    return -1;
	} else if (select_result == 0) {
	    syslog(LOG_INFO, "timeout");
	    return 0;
	}

	errno = 0;
	read_result = read(file, buffer_p, (size_t)rest_length);
	if (read_result < 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	} else if (read_result == 0) {
	    syslog(LOG_ERR, "read() EOF received");
	    return length - rest_length;
	} else {
	    rest_length -= read_result;
	    buffer_p += read_result;
	}
    }

    return 1;
}


/*
 * Write data to a file.
 * It repeats to call write() until all data will have written.
 * The function returns 1 upon success, 0 upon timeout, -1 upon error. 
 */
int
write_all(file, timeout, buffer, length)
    int file;
    int timeout;
    const VOID *buffer;
    size_t length;
{
    const char *buffer_p = buffer;
    ssize_t rest_length = length;
    fd_set fdset;
    struct timeval timeval;
    int select_result;
    ssize_t write_result;

    while (0 < rest_length) {
	errno = 0;
	FD_ZERO(&fdset);
	FD_SET(file, &fdset);

	if (timeout == 0)
	    select_result = select(file + 1, NULL, &fdset, NULL, NULL);
	else {
	    timeval.tv_sec = timeout;
	    timeval.tv_usec = 0;
	    select_result = select(file + 1, NULL, &fdset, NULL, &timeval);
	}
	if (select_result < 0) {
	    if (errno == EINTR)
		continue;
	    syslog(LOG_INFO, "failed to select, %s", strerror(errno));
	    return -1;
	} else if (select_result == 0) {
	    return 0;
	}

	errno = 0;
	write_result = write(file, buffer_p, (size_t)rest_length);
	if (write_result < 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	} else {
	    rest_length -= write_result;
	    buffer_p += write_result;
	}
    }

    return 1;
}


/*
 * Write a string to a file.
 * The function returns 1 upon success, 0 upon timeout, -1 upon error. 
 */
int
write_string_all(file, timeout, string)
    int file;
    int timeout;
    const char *string;
{
    return write_all(file, timeout, string, strlen(string));
}



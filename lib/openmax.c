/*
 * Copyright (c) 1997, 98, 2000, 01  
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
 *   AC_CHECK_HEADERS(unistd.h)
 *   AC_CHECK_FUNCS(getdtablesize, getrlimit, sysconf)
 *   AC_HEADER_TIME
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifdef HAVE_GETDTABLESIZE
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#else /* not HAVE_GETDTABLESIZE */
#ifdef HAVE_GETRLIMIT
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
#include <sys/resource.h>
#else /* not HAVE_GETRLIMIT */
#ifdef HAVE_SYSCONF
#include <limits.h>
#else /* not HAVE_SYSCONF */
#include <sys/param.h>
#ifndef NOFILE
#define DEFAULT_OPEN_MAX  128
#endif /* not NOFILE */
#endif /* not HAVE_SYSCONF */
#endif /* not HAVE_GETRLIMIT */
#endif /* not HAVE_GETDTABLESIZE */


/*
 * Get the maximum number of open files.
 */
int
get_open_max()
{
    int open_max;

#ifdef HAVE_GETDTABLESIZE
    open_max = getdtablesize();
#else /* not HAVE_GETDTABLESIZE */
#ifdef HAVE_GETRLIMIT
    {
	struct rlimit limit;

	getrlimit(RLIMIT_NOFILE, &limit);
	open_max = limit.rlim_cur;
    }
#else /* not HAVE_GETRLIMIT */
#ifdef HAVE_SYSCONF
    open_max = sysconf(_SC_OPEN_MAX);
#else /* not HAVE_SYSCONF */
#ifdef NOFILE
    open_max = NOFILE;
#else /* not NOFILE */
    open_max = DEFAULT_OPEN_MAX;
#endif /* not NOFILE */
#endif /* not HAVE_SYSCONF */
#endif /* not HAVE_GETRLIMIT */
#endif /* not HAVE_GETDTABLESIZE */

    return open_max;
}


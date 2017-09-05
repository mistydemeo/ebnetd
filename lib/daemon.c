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
 *   AC_TYPE_PID_T
 *   AC_CHECK_HEADERS(fcntl.h, unistd.h)
 *   AC_CHECK_FUNCS(setsid)
 *   AC_FUNC_SETPGRP
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <syslog.h>
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_SETSID
#include <sys/ioctl.h>
#endif

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif


/*
 * Run the current process as daemon.
 */
int
daemonize()
{
    pid_t pid;
#ifndef HAVE_SETSID
    int fd = -1;
#endif

    /*
     * Fork, and then the parent exits.
     */
    pid = fork();
    if (pid < 0) {
	syslog(LOG_ERR, "fork() failed, %s", strerror(errno));
	goto failed;
    } else if (0 < pid)
	exit(0);

    /*
     * Close a tty.
     */
#ifdef HAVE_SETSID
    if ((int)setsid() < 0) {
	syslog(LOG_ERR, "setsid() failed, %s", strerror(errno));
	goto failed;
    }
#else
#ifdef SETPGRP_VOID
    setpgrp();
#else /* not SETPGRP_VOID */
    if (setpgrp(0, getpid()) < 0) {
	syslog(LOG_ERR, "setpgrp() failed, %s", strerror(errno));
	goto failed;
    }
#endif /* not SETPGRP_VOID */
#ifdef TIOCNOTTY
    fd = open("/dev/tty" , O_RDWR);
    if (0 <= fd) {
        if (ioctl(fd, TIOCNOTTY, NULL) < 0) {
	    syslog(LOG_ERR, "ioctl(TIOCNOTTY) failed", strerror(errno));
	    goto failed;
	}
	if (close(fd) < 0) {
	    syslog(LOG_ERR, "failed to close the file, %s: /dev/tty",
		strerror(errno));
	    fd = -1;
	    goto failed;
	}
    }
#endif /* TIOCNOTTY */
#endif /* HAVE_SETSID */

    /*
     * Change the current directory to root (`/').
     */
    if (chdir("/") < 0) {
	syslog(LOG_ERR, "failed to change directory to root, %s",
	    strerror(errno));
	goto failed;
    }

    syslog(LOG_DEBUG, "debug: run as daemon");
    return 0;

    /*
     * An error occurs...
     */
  failed:
#ifndef HAVE_SETSID
    if (0 <= fd && close(fd) < 0) {
	syslog(LOG_ERR, "failed to close the file, %s: /dev/tty",
	    strerror(errno));
    }
#endif

    syslog(LOG_ERR, "failed to run as daemon");
    return -1;
}


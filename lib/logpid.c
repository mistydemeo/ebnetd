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
 *   AC_C_CONST
 *   AC_TYPE_PID_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(unistd.h, stdlib.h)
 *   AC_CHECK_FUNCS(strtol)
 *   AC_HEADER_STAT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifndef HAVE_STRTOL
#ifdef __STDC__
long strtol(const char *, char **, int);
#else /* not __STDC__ */
long strtol();
#endif /* not __STDC__ */
#endif /* not HAVE_STRTOL */

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

#ifdef STAT_MACROS_BROKEN
#ifdef S_ISREG
#undef S_ISREG
#endif
#ifdef S_ISDIR
#undef S_ISDIR
#endif
#endif /* STAT_MACROS_BROKEN */

#ifndef S_ISREG
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define isupper(c) ('A' <= (c) && (c) <= 'Z')
#define islower(c) ('a' <= (c) && (c) <= 'z')
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isupper(c) || islower(c) || isdigit(c))
#define isxdigit(c) \
 (isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define toupper(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define tolower(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))

/*
 * Maximum Line length of a pid file.
 */
#define LOGPID_SIZE_LINE		63


/*
 * Log the PID of the current process to file specified at `file_name'.
 * If succeed, 0 is returned.  Otherwise -1 is returned.
 */
int
log_pid_file(file_name)
    const char *file_name;
{
    struct stat status;
    FILE *file;

    /*
     * Does the file already exist?
     */
    if (stat(file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	syslog(LOG_NOTICE, "warning: the PID file already exists: %s",
	    file_name);
    }

    /*
     * Open the file.
     */
    file = fopen(file_name, "w");
    if (file == NULL) {
	syslog(LOG_ERR, "failed to open the file, %s: %s", strerror(errno),
	    file_name);
	goto failed;
    }

    /*
     * Write a PID of the process.
     */
    fprintf(file, "%d\n", (int)getpid());
    if (ferror(file)) {
	syslog(LOG_ERR, "failed to write to the file, %s: %s", strerror(errno),
	    file_name);
	goto failed;
    }

    /*
     * Close the file.
     */
    if (fclose(file) == EOF) {
	syslog(LOG_ERR, "failed to close the file, %s: %s", strerror(errno),
	    file_name);
	file = NULL;
	goto failed;
    }

    syslog(LOG_DEBUG, "debug: write a PID to the file: %s", file_name);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (file != NULL && fclose(file) == EOF) {
	syslog(LOG_ERR, "failed to close the file, %s: %s", strerror(errno),
	    file_name);
    }
    syslog(LOG_ERR, "failed to write a PID to the file: %s", file_name);
    return -1;
}


/*
 * Remove a PID file.
 * If succeed, 0 is returned.  Otherwise -1 is returned.
 */
int
remove_pid_file(file_name)
    const char *file_name;
{
    if (unlink(file_name) == 0) {
	syslog(LOG_DEBUG, "debug: remove the PID file: %s", file_name);
	return 0;
    } else if (errno == ENOENT) {
	syslog(LOG_NOTICE, "warning: the PID file not exist: %s", file_name);
	return 0;
    }

    syslog(LOG_ERR, "unlink failed(), %s: %s", strerror(errno), file_name);
    syslog(LOG_ERR, "failed to remove the PID file: %s", file_name);
    return -1;
}


/*
 * Check for existance of a pid file.
 * If exists, 0 is returned.  Otherwise -1 is returned.
 */
int
probe_pid_file(file_name)
    const char *file_name;
{
    struct stat status;

    /*
     * Does the file already exist?
     */
    if (stat(file_name, &status) == 0 || S_ISREG(status.st_mode))
	return 0;

    return -1;
}


/*
 * Read a PID from a pid file.
 * If succeed, 0 is returned.  Otherwise -1 is returned.
 */
int
read_pid_file(file_name, pid)
    const char *file_name;
    pid_t *pid;
{
    FILE *file;
    char buffer[LOGPID_SIZE_LINE + 1];
    char *end_p;

    /*
     * Open the file.
     */
    file = fopen(file_name, "r");
    if (file == NULL) {
	syslog(LOG_ERR, "failed to open the file, %s: %s", strerror(errno),
	    file_name);
	goto failed;
    }

    /*
     * Read a PID string from the file.
     */
    if (fgets(buffer, LOGPID_SIZE_LINE + 1, file) == NULL) {
	syslog(LOG_ERR, "failed to read the file, %s: %s", strerror(errno),
	    file_name);
	goto failed;
    }

    /*
     * Parse the PID string. 
     */
    *pid = strtol(buffer, &end_p, 10);
    if (!isdigit(buffer[0]) || (*end_p != '\0' && *end_p != '\n')) {
	syslog(LOG_ERR, "invalid PID: %s", file_name);
	goto failed;
    }
 
    /*
     * Close the file.
     */
    if (fclose(file) == EOF) {
	syslog(LOG_ERR, "failed to close the file, %s: %s", strerror(errno),
	    file_name);
	file = NULL;
	goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (file != NULL && fclose(file) == EOF) {
	syslog(LOG_ERR, "failed to close the file, %s: %s", strerror(errno),
	    file_name);
    }
    syslog(LOG_ERR, "failed to read a PID from the file: %s", file_name);
    return -1;
}


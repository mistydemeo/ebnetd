/*
 * Copyright (c) 1997, 98, 99, 2000, 01  
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
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, syslog.h)
 *   AC_CHECK_FUNCS(vsyslog, strerror, syslog)
 *   AC_FUNC_VPRINTF
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
#include <stdarg.h>
#else /* not __STDC__ */
#include <varargs.h>
#endif /* not __STDC__ */
#endif /* (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

#ifndef HAVE_STRERROR
#ifdef __STDC__
char *strerror(int);
#else /* not __STDC__ */
char *strerror();
#endif /* not __STDC__ */
#endif /* HAVE_STRERROR */

#include "fakelog.h"

#undef syslog

/*
 * Internal log priorities.
 */
#define FAKELOG_QUIET		0
#define FAKELOG_EMERG		1
#define FAKELOG_ALERT		2
#define FAKELOG_CRIT		3
#define FAKELOG_ERR		4
#define FAKELOG_WARNING		5
#define FAKELOG_NOTICE		6
#define FAKELOG_INFO		7
#define FAKELOG_DEBUG		8
#define FAKELOG_UNKNOWN		9

/*
 * Log name, mode and level.
 */
static char *log_name = NULL;
static char log_name_buffer[FAKELOG_MAX_LOG_NAME_LENGTH + 1];
static int log_mode = FAKELOG_TO_SYSLOG;
static int log_level = FAKELOG_ERR;

/*
 * Set log name.
 */
void
set_fakelog_name(name)
    const char *name;
{
    if (name == NULL)
        log_name = NULL;
    else {
        strncpy(log_name_buffer, name, FAKELOG_MAX_LOG_NAME_LENGTH);
        log_name_buffer[FAKELOG_MAX_LOG_NAME_LENGTH] = '\0';
        log_name = log_name_buffer;
    }
}


/*
 * Set log level.
 */
void
set_fakelog_mode(mode)
    int mode;
{
    log_mode = mode;
}


/*
 * Set log level.
 */
void
set_fakelog_level(level)
    int level;
{
    /*
     * Convert a syslog priority to a fakelog priority.
     */
    switch (level) {
#ifdef LOG_EMERG
    case LOG_EMERG:
	log_level = FAKELOG_EMERG;
	break;
#endif
#ifdef LOG_ALERT
    case LOG_ALERT:
	log_level = FAKELOG_ALERT;
	break;
#endif
#ifdef LOG_CRIT
    case LOG_CRIT:
	log_level = FAKELOG_CRIT;
	break;
#endif
#ifdef LOG_ERR
    case LOG_ERR:
	log_level = FAKELOG_ERR;
	break;
#endif
#ifdef LOG_WARNING
    case LOG_WARNING:
	log_level = FAKELOG_WARNING;
	break;
#endif
#ifdef LOG_NOTICE
    case LOG_NOTICE:
	log_level = FAKELOG_NOTICE;
	break;
#endif
#ifdef LOG_INFO
    case LOG_INFO:
	log_level = FAKELOG_INFO;
	break;
#endif
#ifdef LOG_DEBUG
    case LOG_DEBUG:
	log_level = FAKELOG_DEBUG;
	break;
#endif
    }
}


/*
 * The function fakes syslog(), and output a message to standard 
 * error, instead of syslog.
 *
 * To use the function, you can fake your sources by the following
 * trick:
 *
 *     #define syslog fakelog
 *
 * Note that this function doesn't support `%m' that syslog() expands
 * into the current error message.
 */
#define BUFFER_SIZE    1024

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
void
fakelog(int priority, const char *message, ...)
#else /* not __STDC__ */
void
fakelog(priority, message, va_alist)
    int priority;
    char *message;
    va_dcl 
#endif /* not __STDC__ */
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
void
fakelog(priority, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)
    int priority;
    char *message;
    char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
{

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
    va_list ap;
#endif
    int log_flag;

    /*
     * Convert `priority'.
     */
    switch (priority) {
#ifdef LOG_EMERG
    case LOG_EMERG:
	log_flag = (FAKELOG_EMERG <= log_level);
	break;
#endif
#ifdef LOG_ALERT
    case LOG_ALERT:
	log_flag = (FAKELOG_ALERT <= log_level);
	break;
#endif
#ifdef LOG_CRIT
    case LOG_CRIT:
	log_flag = (FAKELOG_CRIT <= log_level);
	break;
#endif
#ifdef LOG_ERR
    case LOG_ERR:
	log_flag = (FAKELOG_ERR <= log_level);
	break;
#endif
#ifdef LOG_WARNING
    case LOG_WARNING:
	log_flag = (FAKELOG_WARNING <= log_level);
	break;
#endif
#ifdef LOG_NOTICE
    case LOG_NOTICE:
	log_flag = (FAKELOG_NOTICE <= log_level);
	break;
#endif
#ifdef LOG_INFO
    case LOG_INFO:
	log_flag = (FAKELOG_INFO <= log_level);
	break;
#endif
#ifdef LOG_DEBUG
    case LOG_DEBUG:
	log_flag = (FAKELOG_DEBUG <= log_level);
	break;
#endif
    default:
	log_flag = 0;
    }

    /*
     * Start to use the variable length arguments.
     */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
    va_start(ap, message);
#else /* not __STDC__ */
    va_start(ap);
#endif /* not __STDC__ */
#endif /* (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

    /*
     * Output the message to syslog.
     */
    if (log_mode == FAKELOG_TO_SYSLOG || log_mode == FAKELOG_TO_BOTH) {
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
	vsyslog(priority, message, ap);
#else /* not (defined(HAVE_VPRINTF) || ... */
#ifdef HAVE_SYSLOG
	syslog(priority, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif /* HAVE_SYSLOG */
#endif /* not (defined(HAVE_VPRINTF) || ... */
    }

    /*
     * Output the message to standard error.
     */
    if ((log_mode == FAKELOG_TO_STDERR || log_mode == FAKELOG_TO_BOTH)
	&& log_flag) {
	if (log_name != NULL)
	    fprintf(stderr, "%s: ", log_name);

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef HAVE_VPRINTF
	vfprintf(stderr, message, ap);
#else /* not HAVE_VPRINTF */
#ifdef HAVE_DOPRNT
	_doprnt(message, &ap, stderr);
#endif /* not HAVE_DOPRNT */
#endif /* not HAVE_VPRINTF */
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
	fprintf(stderr, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

	fputc('\n', stderr);
	fflush(stderr);
    }

    /*
     * End to use the variable length arguments.
     */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
    va_end(ap);
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
}



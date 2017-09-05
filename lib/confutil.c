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
 *   AC_TYPE_SIZE_T
 *   AC_TYPE_UID_T
 *   AC_TYPE_GID_T
 *   AC_TYPE_IN_PORT_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, stdlib.h, unistd.h)
 *   AC_CHECK_FUNCS(strcasecmp, strtol)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp();
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

#ifndef HAVE_STRTOL
#ifdef __STDC__
long strtol(const char *, char **, int);
#else /* not __STDC__ */
long strtol();
#endif /* not __STDC__ */
#endif /* not HAVE_STRTOL */


/*
 * Convert a string to an integer.
 *
 * The argument `string' must consists of an optional sign ('-' or '+')
 * and at least one digit ('0'..'9').
 *
 * If `string' doesn't match this format, -1 is returned.
 * Otherwise 0 is returned, and the corresponding integer value is
 * put into `number'.
 */
int
parse_integer(string, number)
    const char *string;
    int *number;
{
    const char *p = string;

    if (*p == '-' || *p == '+')
	p++;
    if (*p == '\0')
	return -1;
    while ('0' <= *p && *p <= '9')
	p++;
    if (*p != '\0')
	return -1;

    *number = (int)strtol(string, NULL, 10);
    return 0;
}


/*
 * Convert a string to a port number. (TCP)
 *
 * The argument `string' must represents a service name on TCP (e.g. `smtp')
 * or a port number (e.g. `25').
 * 
 * If the string doesn't represent neither valid service name nor port
 * number, -1 is returned.  Otherwise, 0 is returned and the corresponding
 * port number is put into `port'.
 */
int
parse_tcp_port(string, port)
    const char *string;
    in_port_t *port;
{
    struct servent *serv;
    int n;

    if (parse_integer(string, &n) == 0) {
	if (n < 0 || 65535 < n)
	    return -1;
	*port = n;
    } else {
	serv = getservbyname(string, "tcp");
	if (serv == NULL)
	    return -1;
	*port = ntohs(serv->s_port);
    }
    return 0;
}


/*
 * Convert a string to a port number. (UDP)
 *
 * The argument `string' must represents a service name on UDP (e.g. `ntp')
 * or a port number (e.g. `123').
 * 
 * If the string doesn't represent neither valid service name nor port
 * number, -1 is returned.  Otherwise, 0 is returned and the corresponding
 * port number is put into `port'.
 */
int
parse_udp_port(string, port)
    const char *string;
    in_port_t *port;
{
    struct servent *serv;
    int n;

    if (parse_integer(string, &n) == 0) {
	if (n < 0 || 65535 < n)
	    return -1;
	*port = n;
    } else {
	serv = getservbyname(string, "udp");
	if (serv == NULL)
	    return -1;
	*port = ntohs(serv->s_port);
    }
    return 0;
}


/*
 * Convert a string to an user ID.
 *
 * The argument `string' must represents an user name (e.g. `root') or
 * an user ID (e.g. `0').
 * 
 * If the string doesn't represent neither valid service name nor port
 * number, -1 is returned.  Otherwise, 0 is returned and the corresponding
 * user ID is put into `port'.
 */
int
parse_user(string, user)
    const char *string;
    uid_t *user;
{
    struct passwd *entry;
    int n;

    if (0 <= parse_integer(string, &n))
	*user = (uid_t) n;
    else {
	entry = getpwnam(string);
	if (entry == NULL)
	    return -1;
	*user = entry->pw_uid;
    }
    return 0;
}


/*
 * Convert a string to an group ID.
 *
 * The argument `string' must represents a group name (e.g. `sys') or an
 * group ID (e.g. `1').
 * 
 * If the string doesn't represent neither valid service name nor port
 * number, -1 is returned.  Otherwise, 0 is returned and the corresponding
 * group ID is put into `port'.
 */
int
parse_group(string, group)
    const char *string;
    gid_t *group;
{
    struct group *entry;
    int n;

    if (0 <= parse_integer(string, &n))
	*group = (gid_t) n;
    else {
	entry = getgrnam(string);
	if (entry == NULL)
	    return -1;
	*group = entry->gr_gid;
    }
    return 0;
}


/*
 * Convert a string (`ON' or `OFF') to an integer (0 or 1).
 *
 * The argument `string' must represents `ON' or `OFF'.  Cases in the
 * string are insensitive, so that `ON', `On' `oN', and `on' are accepted.
 *
 * If succeeded, the result is put into `result' and 0 is returned.
 * If `string' represents ON, `result' is set to 1.
 * If `string' represents OFF, `result' is set to 0.
 * If `string' represents neither ON nor OFF, -1 is returned.
 */
int
parse_on_off(string, result)
    const char *string;
    int *result;
{
    if (strcasecmp(string, "ON") == 0)
	*result = 1;
    else if (strcasecmp(string, "OFF") == 0)
	*result = 0;
    else
	return -1;

    return 0;
}


/*
 * Syslog facility and its name.
 */
typedef struct {
    int code;
    const char *name;
} Facility_Entry;


static const Facility_Entry facility_table[] = {
#ifdef LOG_AUTH
    {LOG_AUTH, "auth"},
#endif
#ifdef LOG_AUTHPRIV
    {LOG_AUTHPRIV, "authpriv"},
#endif
#ifdef LOG_CRON
    {LOG_CRON, "cron"},
#endif
#ifdef LOG_DAEMON
    {LOG_DAEMON, "daemon"},
#endif
#ifdef LOG_FTP
    {LOG_FTP, "ftp"},
#endif
#ifdef LOG_KERN
    {LOG_KERN, "kern"},
#endif
#ifdef LOG_LOCAL0
    {LOG_LOCAL0, "local0"},
#endif
#ifdef LOG_LOCAL1
    {LOG_LOCAL1, "local1"},
#endif
#ifdef LOG_LOCAL2
    {LOG_LOCAL2, "local2"},
#endif
#ifdef LOG_LOCAL3
    {LOG_LOCAL3, "local3"},
#endif
#ifdef LOG_LOCAL4
    {LOG_LOCAL4, "local4"},
#endif
#ifdef LOG_LOCAL5
    {LOG_LOCAL5, "local5"},
#endif
#ifdef LOG_LOCAL6
    {LOG_LOCAL6, "local6"},
#endif
#ifdef LOG_LOCAL7
    {LOG_LOCAL7, "local7"},
#endif
#ifdef LOG_LPR
    {LOG_LPR, "lpr"},
#endif
#ifdef LOG_MAIL
    {LOG_MAIL, "mail"},
#endif
#ifdef LOG_NEWS
    {LOG_NEWS, "news"},
#endif
#ifdef LOG_SYSLOG
    {LOG_SYSLOG, "syslog"},
#endif
#ifdef LOG_USER
    {LOG_USER, "user"},
#endif
#ifdef LOG_UUCP
    {LOG_UUCP, "uucp"},
#endif
    {-1, NULL}
};

/*
 * Convert a syslog facility name to a code.
 *
 * The argument `string' must represents a syslog facility name (e.g.
 * `auth').
 * 
 * If the string doesn't represent valid facility name, -1 is returned.
 * Otherwise, 0 is returned and the corresponding facility code (e.g.
 * LOG_AUTH) is put into `code'.
 */
int
parse_syslog_facility(name, code)
    const char *name;
    int *code;
{
#ifdef LOG_DAEMON
    const Facility_Entry *ent;

    for (ent = facility_table; ent->name != NULL; ent++) {
	if (strcasecmp(name, ent->name) == 0) {
	    *code = ent->code;
	    return 0;
	}
    }
#endif /* not LOG_DAEMON */

    return -1;
}



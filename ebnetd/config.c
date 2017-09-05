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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

/*
 * The maximum length of path name.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#if defined(NDTPD) || defined(EBHTTPD)
#include <eb/eb.h>
#include <eb/appendix.h>
#endif
#if defined(EBNETD)
#include "ebtiny/eb.h"
#include "ebtiny/appendix.h"
#endif

#include "confutil.h"
#include "fakelog.h"
#include "filename.h"
#include "hostname.h"
#include "readconf.h"
#include "permission.h"

#include "defs.h"

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
 * Unexported functions.
 */
static int cf_begin             EBNETD_P((const char *, const char *, int));
static int cf_end               EBNETD_P((const char *, const char *, int));
static int cf_ebnet_port        EBNETD_P((const char *, const char *, int));
static int cf_ndtp_port         EBNETD_P((const char *, const char *, int));
static int cf_http_port         EBNETD_P((const char *, const char *, int));
static int cf_user              EBNETD_P((const char *, const char *, int));
static int cf_group             EBNETD_P((const char *, const char *, int));
static int cf_max_clients       EBNETD_P((const char *, const char *, int));
static int cf_hosts             EBNETD_P((const char *, const char *, int));
static int cf_timeout           EBNETD_P((const char *, const char *, int));
static int cf_work_path         EBNETD_P((const char *, const char *, int));
static int cf_max_hits          EBNETD_P((const char *, const char *, int));
static int cf_max_text_size     EBNETD_P((const char *, const char *, int));
static int cf_syslog_facility   EBNETD_P((const char *, const char *, int));
static int cf_begin_book        EBNETD_P((const char *, const char *, int));
static int cf_end_book          EBNETD_P((const char *, const char *, int));
static int cf_book_name         EBNETD_P((const char *, const char *, int));
static int cf_book_title        EBNETD_P((const char *, const char *, int));
static int cf_book_path         EBNETD_P((const char *, const char *, int));
static int cf_appendix_path     EBNETD_P((const char *, const char *, int));
static int cf_book_max_clients  EBNETD_P((const char *, const char *, int));
static int cf_book_hosts        EBNETD_P((const char *, const char *, int));

/*
 * Syntax of the configuration file.
 */
Configuration configuration_table[] = {
    {"", "begin",             cf_begin,             READCONF_ZERO_OR_ONCE, 0},
    {"", "end",               cf_end,               READCONF_ZERO_OR_ONCE, 0},
    /* single directives. */
    {"", "ebnet-port",        cf_ebnet_port,        READCONF_ZERO_OR_ONCE, 0},
    {"", "ndtp-port",         cf_ndtp_port,         READCONF_ZERO_OR_ONCE, 0},
    {"", "http-port",         cf_http_port,         READCONF_ZERO_OR_ONCE, 0},
    {"", "user",              cf_user,              READCONF_ZERO_OR_ONCE, 0},
    {"", "group",             cf_group,             READCONF_ZERO_OR_ONCE, 0},
    {"", "max-clients",       cf_max_clients,       READCONF_ZERO_OR_ONCE, 0},
    {"", "hosts",             cf_hosts,             READCONF_ZERO_OR_MORE, 0},
    {"", "timeout",           cf_timeout,           READCONF_ZERO_OR_ONCE, 0},
    {"", "work-path",         cf_work_path,         READCONF_ZERO_OR_ONCE, 0},
    {"", "max-hits",          cf_max_hits,          READCONF_ZERO_OR_ONCE, 0},
    {"", "max-text-size",     cf_max_text_size,     READCONF_ZERO_OR_ONCE, 0},
    {"", "syslog-facility",   cf_syslog_facility,   READCONF_ZERO_OR_ONCE, 0},
    /* book group directive. */
    {"",     "begin book",    cf_begin_book,        READCONF_ZERO_OR_MORE, 0},
    {"book", "name",          cf_book_name,         READCONF_ONCE,         0},
    {"book", "title",         cf_book_title,        READCONF_ONCE,         0},
    {"book", "path",          cf_book_path,         READCONF_ONCE,         0},
    {"book", "appendix-path", cf_appendix_path,     READCONF_ZERO_OR_ONCE, 0},
    {"book", "max-clients",   cf_book_max_clients,  READCONF_ZERO_OR_ONCE, 0},
    {"book", "hosts",         cf_book_hosts,        READCONF_ZERO_OR_MORE, 0},
    {"",     "end book",      cf_end_book,          READCONF_ZERO_OR_MORE, 0},
    {"",     NULL,            NULL,                 READCONF_ONCE,         0}
};

/*
 * The current configuring book.
 */
static Book *cf_book;


/*
 * The hook is called before reading a configuration file.
 */
static int
cf_begin(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * The length of the file_name "<work-path>/<basename>"
     * must not exceed PATH_MAX.
     */
    if (PATH_MAX
	< strlen(DEFAULT_WORK_PATH) + 1 + MAX_WORK_PATH_BASE_NAME_LENGTH) {
	syslog(LOG_ERR, "internal error, too long DEFAULT_WORK_PATH");
	return -1;
    }
    strcpy(work_path, DEFAULT_WORK_PATH);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s: debug: beginning of the configuration", file_name);

    return 0;
}


/*
 * The hook is called just after reading a configuration file.
 */
static int
cf_end(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * If the configuration file has no `hosts' directive, and if
     * the mode is SERVER_MODE_CHECK, then output a warning message.
     */
    if (server_mode == SERVER_MODE_CHECK
	&& count_permission(&permissions) == 0) {
	syslog(LOG_ERR, "warning: nobody can connect to the server \
because the configuration file has no `hosts' directive");
    }

    /*
     * If the configuration file has no `book' group directive, and if
     * the mode is SERVER_MODE_CHECK, then output a warning message.
     */
    if (server_mode == SERVER_MODE_CHECK && count_book_registry() == 0)
	syslog(LOG_ERR, "warning: no book definition");

    /*
     * Set the pid file_name.
     */
    sprintf(pid_file_name, "%s/%s", work_path, PID_BASE_NAME);
    if (canonicalize_file_name(pid_file_name) < 0)
	return -1;

    /*
     * Set the connection lock file_name.
     */
    sprintf(connection_lock_file_name, "%s/%s", work_path, LOCK_BASE_NAME);

    /*
     * debugging information.
     */
    syslog(LOG_DEBUG, "%s: debug: end of the configuration", file_name);

    return 0;
}

    
/*
 * `ebnet-port' directive.
 */
static int
cf_ebnet_port(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    in_port_t port;

    /*
     * Parse the port number.
     */
    if (parse_tcp_port(argument, &port) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown service: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Set the port number, if this is EBNETD.
     */
#ifdef EBNETD
    listening_port = port;
#endif

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set ebnet-port: %d", file_name,
	line_number, (int)port);

    return 0;
}


/*
 * `ndtp-port' directive.
 */
static int
cf_ndtp_port(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    in_port_t port;

    /*
     * Parse the port number.
     */
    if (parse_tcp_port(argument, &port) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown service: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Set the port number, if this is NDTPD.
     */
#ifdef NDTPD
    listening_port = port;
#endif

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set ndtp-port: %d", file_name,
	line_number, (int)port);

    return 0;
}


/*
 * `http-port' directive.
 */
static int
cf_http_port(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    in_port_t port;

    /*
     * Parse the port number.
     */
    if (parse_tcp_port(argument, &port) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown service: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Set the port number, if this is ebHTTPD.
     */
#ifdef EBHTTPD
    listening_port = port;
#endif

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set http-port: %d", file_name,
	line_number, (int)port);

    return 0;
}


/*
 * `user' directive.
 */
static int
cf_user(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set the user ID.
     */
    if (parse_user(argument, &user_id) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown user: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set user: %d", file_name, line_number,
	(int)user_id);

    return 0;
}


/*
 * `group' directive.
 */
static int
cf_group(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set the group ID.
     */
    if (parse_group(argument, &group_id) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown group: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set group: %d", file_name, line_number,
	(int)group_id);

    return 0;
}


/*
 * `max-clients' directive.
 */
static int
cf_max_clients(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set the max-clients.
     */
    if (parse_integer(argument, &max_clients) < 0) {
	syslog(LOG_ERR, "%s:%d: not an integer: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set max-clients: %d", file_name,
	line_number, max_clients);

    return 0;
}


/*
 * `hosts' directive.
 */
static int
cf_hosts(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Add `argument' to the host permission list.
     */
    if (add_permission(&permissions, argument) < 0) {
	syslog(LOG_ERR, "%s:%d: invalid host, IP address or IP prefix: %s",
	    file_name, line_number, argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: add hosts: %s", file_name, line_number,
	argument);

    return 0;
}


/*
 * `timeout' directive.
 */
static int
cf_timeout(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set the timeout.
     */
    if (parse_integer(argument, &idle_timeout) < 0) {
	syslog(LOG_ERR, "%s:%d: not an integer: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set timeout: %d", file_name,
	line_number, idle_timeout);

    return 0;
}


/*
 * `work-path' directive.
 */
static int
cf_work_path(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Check for the length of the path.
     * The length of `<work-path>/<basename>' must not exceed PATH_MAX.
     */
    if (PATH_MAX < strlen(argument) + 1 + MAX_WORK_PATH_BASE_NAME_LENGTH) {
	syslog(LOG_ERR, "%s:%d: too long path", file_name, line_number);
	return -1;
    }

    /*
     * The file_name must be an absolute path.
     */
    if (*argument != '/') {
	syslog(LOG_ERR, "%s:%d: not an absolute path: %s", file_name,
	    line_number, argument);
	return -1;
    }

    /*
     * Set the working directory path.
     */
    strcpy(work_path, argument);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set work-path: %s", file_name,
	line_number, work_path);

    return 0;
}


/*
 * `max-hits' directive.
 */
static int
cf_max_hits(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set the max hits.
     */
    if (parse_integer(argument, &max_hits) < 0) {
	syslog(LOG_ERR, "%s:%d: not an integer: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set max-hits: %d", file_name,
	line_number, max_hits);

    return 0;
}


/*
 * `max-text-size' directive.
 */
static int
cf_max_text_size(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    int size;

    /*
     * Parse and set the max text size.
     */
    if (parse_integer(argument, &size) < 0) {
	syslog(LOG_ERR, "%s:%d: not an integer: %s", file_name, line_number,
	    argument);
	return -1;
    }
    max_text_size = size;

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set max-text-size: %d", file_name,
	line_number, max_text_size);

    return 0;
}


/*
 * `syslog-facility' directive.
 */
static int
cf_syslog_facility(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Parse and set a syslog facility.
     */
    if (parse_syslog_facility(argument, &syslog_facility) < 0) {
	syslog(LOG_ERR, "%s:%d: unknown syslog facility: %s", file_name,
	    line_number, argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set syslog-facility: %s", file_name,
	line_number, argument);

    return 0;
}


/*
 * `begin book' sequence.
 */
static int
cf_begin_book(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Create a new book node.
     */
    cf_book = add_book();
    if (cf_book == NULL) {
	syslog(LOG_ERR, "%s:%d: memory exhausted", file_name, line_number);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: beginning of the book definition",
	file_name, line_number);

    return 0;
}


/*
 * `end book' sequence.
 */
static int
cf_end_book(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * If the group directive has no `hosts' sub-directive, and if
     * the mode is SERVER_MODE_CHECK, then output a warning message.
     */
    if (server_mode == SERVER_MODE_CHECK
	&& count_permission(&cf_book->permissions) == 0) {
	syslog(LOG_ERR, "%s:%d: warning: nobody can access the book \
because the book definition has no `hosts' sub-directive",
	    file_name, line_number);
    }

    /*
     * Set `max-clients' for the book to 0 when test mode.
     */
    if (server_mode == SERVER_MODE_TEST) {
	cf_book->max_clients = 0;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: end of the book definition",
	file_name, line_number);

    return 0;
}


/*
 * `name' directive (in book structure).
 */
static int
cf_book_name(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    const char *p;

    /*
     * Check for the length of the book name.
     */
    if (MAX_BOOK_NAME_LENGTH < strlen(argument)) {
	syslog(LOG_ERR, "%s:%d: too long book name", file_name, line_number);
	return -1;
    }

    /*
     * The tagname must consist of upper letters, digit, '_', and '-'.
     */
    for (p = argument; *p != '\0'; p++) {
	if (!islower(*p) && !isdigit(*p) && *p != '_' && *p != '-') {
	    syslog(LOG_ERR,
		"%s:%d: the book name contains invalid character: %s",
		file_name, line_number, argument);
	    return -1;
	}
    }

    /*
     * Check for uniqness of book names.
     */
    if (find_book(argument) != NULL) {
	syslog(LOG_ERR, "%s:%d: the book name redefined: %s", file_name,
	    line_number, argument);
	return -1;
    }

    /*
     * Set the book name.
     */
    strcpy(cf_book->name, argument);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set book-name: %s", file_name,
	line_number, cf_book->name);

    return 0;
}


/*
 * `title' directive (in book structure).
 */
static int
cf_book_title(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Check for the length of the title.
     */
    if (MAX_BOOK_TITLE_LENGTH < strlen(argument)) {
	syslog(LOG_ERR, "%s:%d: too long book title", file_name, line_number);
	return -1;
    }

    /*
     * Set the title.
     */
    strcpy(cf_book->title, argument);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set book-title: %s", file_name,
	line_number, cf_book->title);

    return 0;
}    


/*
 * `path' directive (in book structure).
 */
static int
cf_book_path(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    size_t argument_length;

    /*
     * Check for the length of the path.
     */
    argument_length = strlen(argument);
    if (PATH_MAX < argument_length) {
	syslog(LOG_ERR, "%s:%d: too long path", file_name, line_number);
	return -1;
    }

    /*
     * The path must be an absolute path.
     */
    if (*argument != '/') {
	syslog(LOG_ERR, "%s:%d: not an absolute path: %s", file_name,
	    line_number, argument);
	return -1;
    }

    /*
     * Set the book path.
     */
    cf_book->path = (char *)malloc(argument_length + 1);
    if (cf_book->path == NULL) {
	syslog(LOG_ERR, "%s:%d: memory exhausted", file_name, line_number);
	return -1;
    }
    strcpy(cf_book->path, argument);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set book-path: %s", file_name,
	line_number, argument);

    return 0;
}


/*
 * `appendix-path' directive.
 */
static int
cf_appendix_path(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    size_t argument_length;

    /*
     * Check for the length of the path.
     */
    argument_length = strlen(argument);
    if (PATH_MAX < argument_length) {
	syslog(LOG_ERR, "%s:%d: too long appendix path", file_name);
	return -1;
    }

    /*
     * The path must be an absolute path.
     */
    if (*argument != '/') {
	syslog(LOG_ERR, "%s:%d: not an absolute path: %s", file_name,
	    line_number, argument);
	return -1;
    }

    /*
     * Set the book path.
     */
    cf_book->appendix_path = (char *)malloc(argument_length + 1);
    if (cf_book->appendix_path == NULL) {
	syslog(LOG_ERR, "%s:%d: memory exhausted", file_name, line_number);
	return -1;
    }
    strcpy(cf_book->appendix_path, argument);

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set appendix-path: %s", file_name,
	line_number, argument);

    return 0;
}


/*
 * `max-clients' directive (in book structure).
 */
static int
cf_book_max_clients(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * An argumentument represents a remote host name or address.
     */
    if (parse_integer(argument, &cf_book->max_clients) < 0) {
	syslog(LOG_ERR, "%s:%d: not an integer: %s", file_name, line_number,
	    argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: set book-max-clients: %d",
	file_name, line_number, cf_book->max_clients);

    return 0;
}


/*
 * `hosts' directive (in book structure).
 */
static int
cf_book_hosts(argument, file_name, line_number)
    const char *argument;
    const char *file_name;
    int line_number;
{
    /*
     * Add `argument' to the permission list.
     */
    if (add_permission(&cf_book->permissions, argument) < 0) {
	syslog(LOG_ERR, "%s:%d: invalid host, IP address or IP prefix: %s",
	    file_name, line_number, argument);
	return -1;
    }

    /*
     * Debugging information.
     */
    syslog(LOG_DEBUG, "%s:%d: debug: add book-hosts: %s", file_name,
	line_number, argument);

    return 0;
}

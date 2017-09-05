/*
 * Copyright (c) 1997, 99, 2000, 01  
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
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

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

#if defined(NDTPD) || defined(EBHTTPD)
#include <eb/eb.h>
#endif
#if defined(EBNETD)
#include "ebtiny/eb.h"
#endif

#include "confutil.h"
#include "hostname.h"
#include "linebuf.h"
#include "permission.h"

#include "defs.h"

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

/*
 * Actual program name (argv[0]).
 */
const char *invoked_name;

/*
 * Generic name of the program.
 */
const char *program_name;

/*
 * Program version.
 */
const char *program_version;

/*
 * Path to a working directory.
 */
char work_path[PATH_MAX + 1];

/*
 * Configuration file name.
 */
char configuration_file_name[PATH_MAX + 1];

/*
 * Port number the server listens to.
 */
in_port_t listening_port;
in_port_t old_listening_port;

/*
 * File descriptors for the server to listen to the port.
 */
fd_set listening_files;
int max_listening_file;

/*
 * File descriptor to communicate with a client.
 */
int accepted_in_file;
int accepted_out_file;

/*
 * User ID of the server process.
 */
uid_t user_id;

/*
 * Group ID of the server process.
 */
gid_t group_id;

/*
 * Filename to log the process ID of the server.
 */
char pid_file_name[PATH_MAX + 1];
char old_pid_file_name[PATH_MAX + 1];

/*
 * Lock file_name and ticket stock to count connections to the server.
 */
char connection_lock_file_name[PATH_MAX + 1];
Ticket_Stock connection_ticket_stock;

/*
 * The maximum number of connections to the server at once.
 */
int max_clients;

/*
 * The number of the current connections.
 */
int connection_count;

/*
 * Seconds the server closes an idle connection.
 */
int idle_timeout;

/*
 * Host permission list.
 */
Permission permissions;

/*
 * Book registry.
 */
Book *book_registry;
int book_count;

/*
 * The number of hit entries the server give up searching.
 */
int max_hits;

/*
 * The maximum size of text the server may send as a response to a client.
 */
size_t max_text_size;

/*
 * Hostname of the client.
 */
char client_host_name[MAX_HOST_NAME_LENGTH + 1];

/*
 * Address of the client.
 */
char client_address[MAX_HOST_NAME_LENGTH + 1];

/*
 * Running mode of the server.
 * (standalone, inetd, test, check or control)
 */
int server_mode;

/*
 * Syslog facility.
 */
int syslog_facility;

/*
 * Client sockaddr 
 */
#if defined(ENABLE_IPV6)
struct sockaddr_storage client_sockaddr;
#else
struct sockaddr_in client_sockaddr;
#endif

/*
 * Initialize global variables.
 */
int
initialize_global_variables()
{
    initialize_permission(&permissions);
    initialize_ticket_stock(&connection_ticket_stock);
    initialize_book_registry();

    user_id = getuid();
    group_id = getgid();
    max_clients = DEFAULT_MAX_CLIENTS;
    idle_timeout = DEFAULT_IDLE_TIMEOUT;
    max_hits = DEFAULT_MAX_HITS;
    max_text_size = DEFAULT_MAX_TEXT_SIZE;

    if (parse_tcp_port(SERVICE_NAME, &listening_port) < 0)
	listening_port = DEFAULT_PORT;

    return 0;
}


/*
 * Finalize global variables.
 */
void
finalize_global_variables()
{
    finalize_permission(&permissions);
    finalize_ticket_stock(&connection_ticket_stock);
    finalize_book_registry();
}



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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
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

#if defined(HAVE_SYS_WAIT_H) || defined(HAVE_UNION_WAIT)
#include <sys/wait.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_GETCWD
#define getcwd(d,n) getwd(d)
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#ifdef HAVE_UNION_WAIT
#ifndef WTERMSIG
#define WTERMSIG(x)     ((x).w_termsig)
#endif
#ifndef WCOREDUMP
#define WCOREDUMP(x)    ((x).w_coredump)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(x)  ((x).w_retcode)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(x)  (WTERMSIG(x) != 0)
#endif
#ifndef WIFEXITED
#define WIFEXITED(x)    (WTERMSIG(x) == 0)
#endif
#else /* not HAVE_UNION_WAIT */
#ifndef WTERMSIG
#define WTERMSIG(x) ((x) & 0x7f)
#endif
#ifndef WCOREDUMP
#define WCOREDUMP(x) ((x) & 0x80)
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(x) (((x) >> 8) & 0xff)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(x) (WTERMSIG (x) != 0)
#endif
#ifndef WIFEXITED
#define WIFEXITED(x) (WTERMSIG (x) == 0)
#endif
#endif  /* not HAVE_UNION_WAIT */

#ifndef SHUT_RD
#define SHUT_RD 0
#endif
#ifndef SHUT_WR
#define SHUT_WR 1
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#include "confutil.h"
#include "daemon.h"
#include "fakelog.h"
#include "fdset.h"
#include "filename.h"
#include "getopt.h"
#include "hostname.h"
#include "linebuf.h"
#include "logpid.h"
#include "openmax.h"
#include "permission.h"
#include "privilege.h"
#include "serverport.h"
#include "signame.h"
#include "ticket.h"
#include "wildcard.h"

#include "defs.h"

/*
 * Unexported functions.
 */
static void standalone_main EBNETD_P((void));
static void inetd_main EBNETD_P((void));
static void test_main EBNETD_P((void));
static void output_help EBNETD_P((void));
static void set_signal_handler EBNETD_P((int, RETSIGTYPE (*)(int)));
static void block_signal EBNETD_P((int));
static void unblock_signal EBNETD_P((int));
static RETSIGTYPE abort_parent EBNETD_P((int));
static RETSIGTYPE abort_child EBNETD_P((int));
static RETSIGTYPE terminate_parent EBNETD_P((int));
static RETSIGTYPE terminate_child EBNETD_P((int));
static RETSIGTYPE restart_parent EBNETD_P((int));
static RETSIGTYPE wait_child EBNETD_P((int));

/*
 * Command line options.
 */
static const char *short_options = "c:hitv";
static struct option long_options[] = {
    {"configuration-file", required_argument, NULL, 'c'},
    {"help",               no_argument,       NULL, 'h'},
    {"inetd",              no_argument,       NULL, 'i'},
    {"test",               no_argument,       NULL, 't'},
    {"version",            no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
};

/*
 * The flag is set when the server is started.
 */
static int once_started;

/*
 * The flag is set if SIGHUP is received.
 */
static int restart_trigger;

/*
 * Process group ID of the server.
 */
pid_t process_group_id;

/*
 * Signal mask.
 */
#if !defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGSETMASK)
static int signal_mask = 0;
#endif


int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Error_Code error_code;
    int ch;

    /*
     * We must output US-ASCII syslog messages.
     */
#ifdef HAVE_SETENV
    setenv("LC_ALL", "C", 1);
    setenv("LANGUAGE", "C", 1);
#else
    putenv("LC_ALL=C");
    putenv("LANGUAGE=C");
#endif

#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "C");
#endif

    /*
     * Set program name and version.
     */
    invoked_name = argv[0];
    program_name = SERVER_PROGRAM_NAME;
    program_version = VERSION;

    /*
     * Set umask.
     */
    umask(SERVER_UMASK);

    /*
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_SYSLOG);
    set_fakelog_level(LOG_DEBUG);

    /*
     * Open syslog.
     * If LOG_DAEMON is not defined, it assumes old style syslog
     * It doesn't have facility.
     */
    syslog_facility = DEFAULT_SYSLOG_FACILITY;
#ifdef LOG_DAEMON
    openlog(program_name, LOG_NDELAY | LOG_PID, syslog_facility);
#else
    openlog(program_name, LOG_NDELAY | LOG_PID);
#endif
    syslog(LOG_DEBUG, "debug: open syslog");

    /*
     * Initialize EB Library.
     */
    error_code = eb_initialize_library();
    if (error_code != EB_SUCCESS) {
	syslog(LOG_ERR, "%s\n", eb_error_message(error_code));
	exit(1);
    }

    /*
     * Initialize variables.
     */
    once_started = 0;
    initialize_fdset(&listening_files, &max_listening_file);
    server_mode = SERVER_MODE_STANDALONE;

    if (PATH_MAX < strlen(DEFAULT_CONFIGURATION_FILE_NAME)) {
	syslog(LOG_ERR,
	    "internal error, too long DEFAULT_CONFIGURATION_FILE_NAME");
	goto die;
    }
    strcpy(configuration_file_name, DEFAULT_CONFIGURATION_FILE_NAME);
    initialize_global_variables();

    /*
     * Parse command line options.
     */
    for (;;) {
	ch = getopt_long(argc, argv, short_options, long_options, NULL);
	if (ch == -1)
	    break;
	switch (ch) {
	case 'c':
	    /*
	     * Option `-c file'.  Specify the configuration file name.
	     */
	    if (PATH_MAX < strlen(optarg)) {
		fprintf(stderr, "%s: too long configuration file name\n",
		    invoked_name);
		goto die;
	    }
	    strcpy(configuration_file_name, optarg);
	    if (canonicalize_file_name(configuration_file_name) < 0)
		goto die;
	    break;

	case 'h':
	    /*
	     * Option `-h'.  Help.
	     */
	    output_help();
	    exit(0);

	case 'i':
	    /*
	     * Option `-i'.  Inetd mode.
	     */
	    server_mode = SERVER_MODE_INETD;
	    break;

	case 't':
	    /*
	     * Option `-t'.  Test mode.
	     */
	    server_mode = SERVER_MODE_TEST;
	    break;

	case 'v':
	    /*
	     * Option `-v'.  Show version and exit immediately.
	     */
	    output_version();
	    exit(0);

	default:
	    output_try_help();
	    exit(1);
	}
    }

    /*
     * Check for the number of rest arguments.
     */
    if (0 < argc - optind) {
	fprintf(stderr, "%s: too many arguments\n", invoked_name);
	output_try_help();
	exit(1);
    }

    /*
     * Main routine for each server mode.
     */
    switch (server_mode) {
    case SERVER_MODE_STANDALONE:
	for (;;)
	    standalone_main();
	break;
    case SERVER_MODE_INETD:
	inetd_main();
	break;
    case SERVER_MODE_TEST:
	test_main();
	break;
    }

    return 0;

    /*
     * A fatal error occurrs...
     */
  die:
    if (server_mode == SERVER_MODE_STANDALONE)
	syslog(LOG_CRIT, "the server exits");
    else if (server_mode == SERVER_MODE_INETD)
	syslog(LOG_INFO, "the server exits");

    exit(1);
}


/*
 * Main routine for standalone mode.
 */
static void
standalone_main()
{
#if defined(ENABLE_IPV6)
    struct sockaddr_storage address;
#else
    struct sockaddr_in address;
#endif
    socklen_t address_length;
    int fd;
    int pid = -1;

    fd_set select_files;
    int i;

    /*
     * Block signals.
     */
#if !defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGSETMASK)
    signal_mask = 0;
#endif
    block_signal(SIGHUP);
    block_signal(SIGINT);
    block_signal(SIGTERM);

    /*
     * Reset restart trigger.
     */
    restart_trigger = 0;
    
    /*
     * Kill child processes.
     */
    if (once_started) {
#ifdef HAVE_KILLPG
	killpg(process_group_id, SIGINT);
#else
	kill(-process_group_id, SIGINT);
#endif
	syslog(LOG_DEBUG, "debug: kill all child processes");
    }

    /*
     * Close unwanted files.
     */
    if (!once_started) {
	closelog();
	for (fd = get_open_max() - 1; 0 <= fd; fd--)
	    close(fd);
#ifdef LOG_DAEMON
	openlog(program_name, LOG_NDELAY | LOG_PID, syslog_facility);
#else
	openlog(program_name, LOG_NDELAY | LOG_PID);
#endif
	syslog(LOG_DEBUG, "debug: reopen syslog");
    }

    /*
     * Re-initinalize variables.
     */
    if (once_started) {
	finalize_global_variables();
	initialize_global_variables();
    }

    /*
     * Read a configuration file.
     */
    if (read_configuration(configuration_file_name, configuration_table) < 0) {
	syslog(LOG_ERR, "configuration failure");
	goto die;
    }

    /*
     * Reopen syslog.
     */
    closelog();
#ifdef LOG_DAEMON
    openlog(program_name, LOG_NDELAY | LOG_PID, syslog_facility);
#else
    openlog(program_name, LOG_NDELAY | LOG_PID);
#endif
    /*
     * Run as standalone daemon.
     */
    if (!once_started) {
	if (daemonize() < 0)
	    goto die;
#ifdef GETPGRP_VOID
	process_group_id = getpgrp();
#else
	process_group_id = getpgrp(getpid());
#endif
    }

    /*
     * Change the current working directory.
     */
    if (chdir(work_path) < 0) {
	syslog(LOG_ERR, "failed to change the directory, %s: %s", 
	    strerror(errno), work_path);
	goto die;
    }

    /*
     * Shutdown old connections if the server port is changed.
     */
    if (once_started && listening_port != old_listening_port) {
	/* shutdown multi sockets */
	if (shutdown_fdset(&listening_files, &max_listening_file, SHUT_RDWR)
	    < 0) {
	    syslog(LOG_ERR, "shutdown() failed, %s", strerror(errno));
	    goto die;
	}
	/* close multi sockets */
	close_fdset(&listening_files, &max_listening_file);
	syslog(LOG_DEBUG, "debug: shutdown the old socket: %d/tcp",
	    listening_port);
    }


    /*
     * Bind and listen to the server port. 
     */
    if (max_listening_file < 0) {
	if (set_server_ports(listening_port, LISTENING_BACKLOG,
	    &listening_files, &max_listening_file) < 0)
	    goto die;
	old_listening_port = listening_port;
    }
    /*
     * Set UID and GID.
     */
    if (set_privilege(user_id, group_id) < 0)
	goto die;

    /*
     * Activate all books.
     */
    if (activate_book_registry() == 0)
	syslog(LOG_ERR, "no book is available");
	
    /*
     * Unlink an old PID file if exists.
     */
    if (once_started)
	remove_pid_file(old_pid_file_name);
    strcpy(old_pid_file_name, pid_file_name);

    /*
     * Bind a ticket stock.
     */
    if (max_clients != 0 && bind_ticket_stock(&connection_ticket_stock,
	connection_lock_file_name, max_clients) < 0)
	goto die;

    /*
     * Log pid to the file.
     */
    if (log_pid_file(pid_file_name) < 0)
	goto die;

    /*
     * Set signal handlers.
     */
    set_signal_handler(SIGBUS, abort_parent);
    set_signal_handler(SIGSEGV, abort_parent);
    set_signal_handler(SIGINT, SIG_IGN);
    set_signal_handler(SIGQUIT, SIG_IGN);
    set_signal_handler(SIGTERM, terminate_parent);
    set_signal_handler(SIGCHLD, wait_child);
    set_signal_handler(SIGHUP, restart_parent);

    /*
     * Set `once_started' flag.
     */
    if (once_started)
	syslog(LOG_NOTICE, "server restarted");
    else
	syslog(LOG_NOTICE, "server started");
    once_started = 1;

    /*
     * Main loop for the parent process.
     */
    for (;;) {
	/*
	 * Unblock fatal signals.
	 */
	unblock_signal(SIGHUP);
	unblock_signal(SIGINT);
	unblock_signal(SIGTERM);
	unblock_signal(SIGCHLD);

	/*
	 * Wait a connection from a client.
	 */ 
	if (wait_server_ports(&listening_files, &select_files,
	    &max_listening_file) < 0) {
	    if (errno != EINTR) {
		syslog(LOG_DEBUG, "failed to wait a connection, %s",
		    strerror(errno));
		goto die;
	    }
	    else if (restart_trigger) {
		return;
	    } else {
		continue;
	    }
   
	}
	/*
	 * Block fatal signals.
	 */
	block_signal(SIGHUP);
	block_signal(SIGINT);
	block_signal(SIGTERM);
	block_signal(SIGCHLD);
	
	/*
	 * Accept a connection.
	 */
#if defined(ENABLE_IPV6)
	address_length = sizeof(struct sockaddr_storage);
#else
	address_length = sizeof(struct sockaddr_in);
#endif

	/*
	 * TODO: nonblocking I/O (not yet)
	 */
	for (i = 0; i <= max_listening_file; i++) {
	    if (FD_ISSET(i, &select_files)) {
		accepted_in_file = accept(i, (struct sockaddr *)&address,
		    &address_length);
		accepted_out_file = accepted_in_file;
		if (accepted_in_file < 0) {
		    syslog(LOG_ERR, "accept() failed, %s", strerror(errno));
		    continue;
		}

		syslog(LOG_DEBUG, "debug: accept the connection");

		/*
		 * Fork.  The child process talk to the client.
		 */
		pid = fork();
		if (pid < 0) {
		    syslog(LOG_ERR, "fork() failed, %s", strerror(errno));
		    continue;
		} else if (pid == 0) {
		    syslog(LOG_DEBUG, "debug: forked");
		    break;
		}

		/*
		 * Close an accepted socket (at the parent process).
		 */
		if (close(accepted_in_file) < 0) {
		    syslog(LOG_ERR, "close() failed, %s", strerror(errno));
		    goto die;
		}
	    }
	}

	if (pid == 0) { 
	    break;
	}
	
    }

    /*
     * Set signal handlers.
     */
    set_signal_handler(SIGBUS, abort_child);
    set_signal_handler(SIGSEGV, abort_child);
    set_signal_handler(SIGINT, terminate_child);
    set_signal_handler(SIGTERM, terminate_child);
    set_signal_handler(SIGCHLD, SIG_IGN);
    set_signal_handler(SIGHUP, terminate_child);
    set_signal_handler(SIGPIPE, SIG_IGN);

    /*
     * Unblock fatal signals.
     */
    unblock_signal(SIGHUP);
    unblock_signal(SIGINT);
    unblock_signal(SIGTERM);
    unblock_signal(SIGCHLD);

    /*
     * Close the file to listen to the server port.
     */
    close_fdset(&listening_files, &max_listening_file);

    /*
     * Get the client host name and address.
     */
    if (identify_remote_host(accepted_in_file, client_host_name,
	client_address) < 0) {
	syslog(LOG_ERR, "connection denied: host=%s(%s)", client_host_name,
	    client_address);
	shutdown(accepted_in_file, SHUT_RDWR);
	exit(1);
    }
    /*
     * Check for the access permission to the client.
     */
    if (!test_permission(&permissions, client_host_name, client_address,
	match_wildcard)) {
	syslog(LOG_ERR, "connection denied: host=%s(%s)",
	    client_host_name, client_address);
	shutdown(accepted_in_file, SHUT_RDWR);
	exit(0);
    }
    /*
     * Get a ticket for the client.
     */
    if (max_clients != 0) {
	if (get_ticket(&connection_ticket_stock) < 0) {
	    syslog(LOG_INFO, "full of clients");
	    shutdown(accepted_in_file, SHUT_RDWR);
	    exit(0);
	}
    }

    syslog(LOG_INFO, "connected: host=%s(%s)", client_host_name,
	client_address);

    /*
     * Set book permissions to the current client.
     */
    check_book_permissions();

    /*
     * Communicate with the client.
     */
    protocol_main();
    shutdown(accepted_in_file, SHUT_RDWR);

    /*
     * Clear lists and buffers.
     */
    finalize_global_variables();
    eb_finalize_library();

    syslog(LOG_INFO, "the child server process exits");
    exit(0);

    /*
     * A fatal error occurrs on a parent...
     */
  die:
    shutdown_fdset(&listening_files, &max_listening_file, SHUT_RDWR);
    close_fdset(&listening_files, &max_listening_file);
    if (once_started)
	remove_pid_file(old_pid_file_name);

    if (once_started) {
	set_signal_handler(SIGINT, SIG_IGN);
#ifdef HAVE_KILLPG
	killpg(process_group_id, SIGINT);
#else
	kill(-process_group_id, SIGINT);
#endif
    }

    finalize_global_variables();
    eb_finalize_library();

    syslog(LOG_CRIT, "the server exits");
    exit(1);
}


/*
 * Main routine for inetd mode.
 */
static void
inetd_main()
{
    accepted_in_file = 0;
    accepted_out_file = 1;

    /*
     * Set signal handlers.
     */
    set_signal_handler(SIGBUS, abort_parent);
    set_signal_handler(SIGSEGV, abort_parent);
    set_signal_handler(SIGINT, SIG_IGN);
    set_signal_handler(SIGQUIT, SIG_IGN);
    set_signal_handler(SIGTERM, terminate_parent);
    set_signal_handler(SIGHUP, terminate_parent);
    set_signal_handler(SIGPIPE, SIG_IGN);

    /*
     * Read a configuration file.
     */
    if (read_configuration(configuration_file_name, configuration_table) < 0) {
	syslog(LOG_ERR, "configuration failure");
	goto die;
    }

    /*
     * Reopen syslog.
     */
    closelog();
#ifdef LOG_DAEMON
    openlog(program_name, LOG_NDELAY | LOG_PID, syslog_facility);
#else
    openlog(program_name, LOG_NDELAY | LOG_PID);
#endif

    /*
     * Change the current working directory.
     */
    if (chdir(work_path) < 0) {
	syslog(LOG_ERR, "failed to change the directory, %s: %s",
	    strerror(errno), work_path);
	goto die;
    }

    /*
     * Set UID and GID.
     */
    if (set_privilege(user_id, group_id) < 0) {
	syslog(LOG_ERR,
	    "failed to set owner and group of the process");
	goto die;
    }

    /*
     * Get the client host name and address.
     */
    if (identify_remote_host(accepted_in_file, client_host_name,
	client_address) < 0) {
	syslog(LOG_ERR, "connection denied: host=%s(%s)", client_host_name,
	    client_address);
	goto die;
    }
    /*
     * Check for the access permission to the client.
     */
    if (!test_permission(&permissions, client_host_name, client_address,
	match_wildcard)) {
	syslog(LOG_ERR, "connection denied: host=%s(%s)",
	    client_host_name, client_address);
	shutdown(accepted_in_file, SHUT_RDWR);
	exit(0);
    }

    /*
     * Get a ticket for the client.
     */
    if (max_clients != 0) {
	bind_ticket_stock(&connection_ticket_stock, connection_lock_file_name,
	    max_clients);
	if (get_ticket(&connection_ticket_stock) < 0) {
	    syslog(LOG_INFO, "full of clients");
	    goto die;
	}
    }

    syslog(LOG_INFO, "connected: host=%s(%s)",
	client_host_name, client_address);

    /*
     * Activate all books.
     */
    if (activate_book_registry() == 0)
	syslog(LOG_ERR, "no book is available");
	
    /*
     * Check book permissions to the current client.
     */
    check_book_permissions();

    /*
     * Communicate with the client.
     */
    protocol_main();

    /*
     * Clear lists and buffers.
     */
    finalize_global_variables();
    eb_finalize_library();

    syslog(LOG_INFO, "the server exits");
    exit(0);

    /*
     * A fatal error occurrs...
     */
  die:
    fflush(stderr);
    shutdown(accepted_in_file, SHUT_RDWR);
    finalize_global_variables();
    eb_finalize_library();

    syslog(LOG_INFO, "the server exits");
    exit(1);
}


/*
 * Main routine for inetd mode.
 */
static void
test_main()
{
    accepted_in_file = 0;
    accepted_out_file = 1;

    /*
     * Read a configuration file.
     */
    if (read_configuration(configuration_file_name, configuration_table) < 0) {
	fprintf(stderr, "configuration failure\n");
	syslog(LOG_ERR, "configuration failure");
	exit(1);
    }

    /*
     * Activate all books.
     */
    if (activate_book_registry() == 0)
	syslog(LOG_ERR, "no book is available");
	
    /*
     * Set dummy host name and address.
     */
    strcpy(client_address, "test-mode");
    if (isatty(0))
	strncpy(client_host_name, ttyname(0), MAX_HOST_NAME_LENGTH);
    else
	strncpy(client_host_name, "stdin", MAX_HOST_NAME_LENGTH);
    *(client_host_name + MAX_HOST_NAME_LENGTH) = '\0';

    idle_timeout = 0;

    /*
     * Reopen syslog.
     */
    closelog();
#ifdef LOG_DAEMON
    openlog(program_name, LOG_NDELAY | LOG_PID, syslog_facility);
#else
    openlog(program_name, LOG_NDELAY | LOG_PID);
#endif

    syslog(LOG_INFO, "connected: host=%s(%s)", client_host_name,
	client_address);

    /*
     * Set book permissions.
     */
    set_all_book_permissions();

    /*
     * Communicate with the client.
     */
    printf("# %s version %s\n", program_name, program_version);
    printf("# test mode\n");
    fflush(stdout);
    protocol_main();

    /*
     * Clear lists and buffers.
     */
    finalize_global_variables();
    eb_finalize_library();

    exit(0);
}


/*
 * Output usage to standard out.
 */
static void
output_help()
{
    printf("Usage: %s [option...]\n", program_name);
    printf("Options:\n");
    printf("  -c FILE  --configuration-file FILE\n");
    printf("                             specify a configuration file\n");
    printf("                             (default: %s)\n",
	DEFAULT_CONFIGURATION_FILE_NAME);
    printf("  -h  --help                 display this help, then exit\n");
    printf("  -i  --inetd                inetd mode\n");
    printf("  -t  --test                 test mode\n");
    printf("  -v  --version              display version number, then exit\n");
    printf("\nDefault value used in a configuration file:\n");
    printf("  work-path                  %s\n", DEFAULT_WORK_PATH);
    printf("\nOptional feature:\n");
#ifdef ENABLE_IPV6
    printf("  IPv6 support               enabled\n");
#else
    printf("  IPv6                       disabled\n\n");
#endif

    printf("\nReport bugs to %s.\n", MAILING_ADDRESS);
    fflush(stdout);
}


/*
 * Set a signal handler.
 * `signal' on some SystemV based systems is not reliable.
 * We use `sigaction' if available.
 */
static void
set_signal_handler(signal_number, function)
    int signal_number;
    RETSIGTYPE (*function) EBNETD_P((int));
{
#ifdef HAVE_SIGPROCMASK
    struct sigaction action;

    action.sa_handler = function;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_NOCLDSTOP;

    if (signal_number == SIGALRM) {
#ifdef SA_INTERRUPT
	action.sa_flags |= SA_INTERRUPT;
#endif
    } else {
#ifdef SA_RESTART
	action.sa_flags |= SA_RESTART;
#endif
    }
    sigaction(signal_number, &action, NULL);

#else /* not HAVE_SIGPROCMASK */
    signal(signal_number, function);
#endif /* not HAVE_SIGPROCMASK */
}


/*
 * Block signals of `signal_number' type (SIGHUP, SIGINT, ...).
 *
 * `sigprocmask' is used, if available.
 * Otherwise, `sigsetmask' is used if the system has it.
 * If neither `sigprocmask' nor `sigsetmask' is available, we give up
 * blocking a signal.
 */
static void
block_signal(signal_number)
    int signal_number;
{
#ifdef HAVE_SIGPROCMASK
    sigset_t signal_set;
#endif

#ifdef HAVE_SIGPROCMASK
    sigemptyset(&signal_set);
    sigaddset(&signal_set, signal_number);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);
#else /* not HAVE_SIGPROCMASK */
#ifdef HAVE_SIGSETMASK
    signal_mask |= sigmask(signal_number);
    sigsetmask(signal_mask);
#endif /* HAVE_SIGSETMASK */
#endif /* not HAVE_SIGPROCMASK */
}


/*
 * Block signals of `signal_number' type (SIGHUP, SIGINT, ...).
 *
 * `sigprocmask' is used, if available.
 * Otherwise, `sigsetmask' is used if the system has it.
 * If neither `sigprocmask' nor `sigsetmask' is available, we give up
 * blocking a signal.
 */
static void
unblock_signal(signal_number)
    int signal_number;
{
#ifdef HAVE_SIGPROCMASK
    sigset_t signal_set;
#endif

#ifdef HAVE_SIGPROCMASK
    sigemptyset(&signal_set);
    sigaddset(&signal_set, signal_number);
    sigprocmask (SIG_UNBLOCK, &signal_set, NULL);
#else /* not HAVE_SIGPROCMASK */
#ifdef HAVE_SIGSETMASK
    signal_mask = (signal_mask | sigmask(signal_number))
	^ sigmask(signal_number);
    sigsetmask(signal_mask);
#endif /* HAVE_SIGSETMASK */
#endif /* not HAVE_SIGPROCMASK */
}


/*
 * Signal handler for SIGBUS and SIGSEGV. (for parent)
 */
static RETSIGTYPE
abort_parent(signal_number)
    int signal_number;
{
    if (server_mode == SERVER_MODE_STANDALONE) {
	syslog(LOG_CRIT, "the server process aborts, receives SIG%s",
	    signal_name(signal_number));
    } else {
	syslog(LOG_ERR, "the server process aborts, receives SIG%s",
	    signal_name(signal_number));
    }
    abort();

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}


/*
 * Signal handler for SIGBUS and SIGSEGV. (for child)
 */
static RETSIGTYPE
abort_child(signal_number)
    int signal_number;
{
    syslog(LOG_ERR, "the child server process aborts, receives SIG%s",
	signal_name(signal_number));
    abort();

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}


/*
 * Signal handler for SIGTERM, SIGINT SIGQUIT, and SIGHUP. (for parent)
 */
static RETSIGTYPE
terminate_parent(signal_number)
    int signal_number;
{
    if (server_mode == SERVER_MODE_STANDALONE) {
	syslog(LOG_CRIT, "the server process exits, receives SIG%s",
	    signal_name(signal_number));
    } else {
	syslog(LOG_ERR, "the server process exits, receives SIG%s",
	    signal_name(signal_number));
    }

    /*
     * Kill child processes.
     */
    set_signal_handler(SIGINT, SIG_IGN);
    if (server_mode == SERVER_MODE_STANDALONE) {
#ifdef HAVE_KILLPG
	killpg(process_group_id, SIGINT);
#else
	kill(-process_group_id, SIGINT);
#endif
    }

    /*
     * Shutdown a server port.
     */
    shutdown_fdset(&listening_files, &max_listening_file, SHUT_RDWR);
    close_fdset(&listening_files, &max_listening_file);
    
    /*
     * Remove a PID file.
     */
    if (server_mode == SERVER_MODE_STANDALONE)
	remove_pid_file(old_pid_file_name);

    _exit(1);

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}


/*
 * Signal handler for SIGHUP, SIGINT, SIGQUIT and SIGTERM. (for child)
 */
static RETSIGTYPE
terminate_child(signal_number)
    int signal_number;
{
    syslog(LOG_ERR, "the child server process exits, receives SIG%s",
	signal_name(signal_number));
    shutdown(accepted_in_file, SHUT_RDWR);

    /*
     * Clear lists and buffers.
     */
    finalize_global_variables();
    eb_finalize_library();

    _exit(0);

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}


/*
 * Signal handler for SIGCHLD.
 */
static RETSIGTYPE
wait_child(signal_number)
    int signal_number;
{
#ifdef HAVE_UNION_WAIT
    union wait status;
#else
    int status;
#endif

    syslog(LOG_INFO, "receives SIGCHLD");

#ifdef HAVE_WAITPID
    while (0 < waitpid(-1, &status, WNOHANG))
	;
#else /* not HAVE WAITPID */
    while (0 < wait3(&status, WNOHANG, NULL))
	;
#endif /* not HAVE WAITPID */

    /*
     * Set a signal handler again.
     */
    set_signal_handler(SIGCHLD, wait_child);

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}


/*
 * Signal handler for SIGHUP (for parent).
 */
static RETSIGTYPE
restart_parent(signal_number)
    int signal_number;
{
    syslog(LOG_ERR, "the server restart, receives SIGHUP");
    restart_trigger++;

#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}

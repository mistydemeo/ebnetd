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
#include <signal.h>
#include <errno.h>

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

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp();
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

#ifndef HAVE_STRERROR
#ifdef __STDC__
char *strerror(int);
#else /* not __STDC__ */
char *strerror();
#endif /* not __STDC__ */
#endif /* HAVE_STRERROR */

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

#include "fakelog.h"
#include "getopt.h"
#include "hostname.h"
#include "logpid.h"
#include "permission.h"
#include "readconf.h"
#include "wildcard.h"

#include "defs.h"

/*
 * Unexported functions.
 */
static void output_help EBNETD_P((void));

/*
 * Command line options.
 */
static const char *short_options = "c:hv";
static struct option long_options[] = {
    {"configuration-file", required_argument, NULL, 'c'},
    {"help",               no_argument,       NULL, 'h'},
    {"version",            no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
};

/*
 * Sub-commands.
 */
#define SUB_COMMAND_STATUS		0
#define SUB_COMMAND_RESTART		1
#define SUB_COMMAND_TERMINATE		2
#define SUB_COMMAND_KILL		3

typedef struct {
    const char *name;
    int id;
    int signal;
} Sub_Command;

static const Sub_Command command_table[] = {
    {"status",    SUB_COMMAND_STATUS,    -1},
    {"restart",   SUB_COMMAND_RESTART,   SIGHUP},
    {"terminate", SUB_COMMAND_TERMINATE, SIGTERM},
    {"kill",      SUB_COMMAND_KILL,      SIGKILL},
    {NULL, 0}
};

/*
 * Unexported functions.
 */
#ifdef __STDC__
static const Sub_Command *get_sub_command(const char *);
#else
static const Sub_Command *get_sub_command();
#endif


int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Error_Code error_code;
    const Sub_Command *command;
    pid_t pid;
    int ch;

    /*
     * We must output US-ASCII syslog messages.
     */
#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "C");
#endif

    /*
     * Set program name and version.
     */
    invoked_name = argv[0];
    program_name = CONTROL_PROGRAM_NAME;
    program_version = VERSION;

    /*
     * Initialize EB Library.
     */
    error_code = eb_initialize_library();
    if (error_code != EB_SUCCESS) {
	syslog(LOG_ERR, "%s\n", eb_error_message(error_code));
	exit(1);
    }

    /*
     * Initialize global and static variables.
     */
    server_mode = SERVER_MODE_CONTROL;
    initialize_global_variables();

    if (PATH_MAX < strlen(DEFAULT_CONFIGURATION_FILE_NAME)) {
	fprintf(stderr,
	    "%s: internal error, too long DEFAULT_CONFIGURATION_FILE_NAME\n",
	    invoked_name);
	exit(1);
    }
    strcpy(configuration_file_name, DEFAULT_CONFIGURATION_FILE_NAME);

    /*
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_STDERR);
    set_fakelog_level(LOG_ERR);

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
	     * Option `-c file'.  Specify a configuration file_name.
	     */
	    if (PATH_MAX < strlen(optarg)) {
		fprintf(stderr, "%s: too long configuration file_name\n",
		    invoked_name);
		fflush(stderr);
		exit(1);
	    }
	    strcpy(configuration_file_name, optarg);
	    break;

	case 'h':
	    /*
	     * Option `-h'.  Help.
	     */
	    output_help();
	    exit(0);

	case 'v':
	    /*
	     * Option `-v'.  Show the version number.
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
    if (argc - optind == 0) {
	fprintf(stderr, "%s: too few argument\n", invoked_name);
	output_try_help();
	exit(1);
    }
    if (1 < argc - optind) {
	fprintf(stderr, "%s: too many arguments\n", invoked_name);
	output_try_help();
	exit(1);
    }

    /*
     * Check for sub-command.
     */
    command = get_sub_command(argv[optind]);
    if (command == NULL) {
	output_try_help();
	exit(1);
    }

    /*
     * Read the configuration file.
     */
    if (read_configuration(configuration_file_name, configuration_table) < 0) {
	fprintf(stderr, "%s: configuration failure\n", invoked_name);
	fflush(stderr);
	exit(1);
    }

    /*
     * Read PID from the pid-file.
     */
    if (probe_pid_file(pid_file_name) < 0) {
	fprintf(stderr, "%s: PID file not found: %s\n", invoked_name,
	    pid_file_name);
	fprintf(stderr, "%s: server not running\n", invoked_name);
	fflush(stderr);
	exit(1);
    }
    if (read_pid_file(pid_file_name, &pid) < 0)
	exit(1);

    /*
     * When sub-command is `status', output a message and exit.
     */
    if (command->id == SUB_COMMAND_STATUS) {
	fprintf(stderr, "%s: PID file found: %s\n", invoked_name,
	    pid_file_name);
	fprintf(stderr, "%s: ndtpd is running (PID = %u)\n", invoked_name,
	    (unsigned)pid);
	fflush(stderr);
	exit(0);
    }

    /*
     * Send a signal to the running `ndtpd'.
     */
    if (kill(pid, command->signal) < 0) {
	fprintf(stderr, "%s: %s\n", invoked_name, strerror(errno));
	fflush(stderr);
	exit(1);
    }

    /*
     * When ndtpd is killed (send SIGKILL), ndtpd failed to remove
     * the pid file ndtpd.  The command try to remove it.
     */
    if (command->id == SUB_COMMAND_KILL) {
	if (remove_pid_file(pid_file_name) < 0)
	    exit(1);
    }

    /*
     * Finalize global and static variables.
     */
    finalize_global_variables();

    /*
     * Finalize EB Library.
     */
    eb_finalize_library();
    
    return 0;
}


/*
 * Parse sub-command.
 */
static const Sub_Command *
get_sub_command(name)
    const char *name;
{
    size_t name_length;
    const Sub_Command *command;
    const Sub_Command *candidate = NULL;
    int matches;
    
    name_length = strlen(name);
    matches = 0;
    for (command = command_table; command->name != NULL; command++) {
	if (strcasecmp(name, command->name) == 0)
	    return command;
	else if (strncasecmp(name, command->name, name_length) == 0) {
	    candidate = command;
	    matches++;
	}
    }

    if (matches == 0) {
	fprintf(stderr, "%s: unknown command `%s'\n", invoked_name, name);
	fflush(stderr);
	return NULL;
    } else if (1 < matches) {
	fprintf(stderr, "%s: command `%s' is ambigous\n", invoked_name, name);
	fflush(stderr);
	return NULL;
    }

    return candidate;
}


/*
 * Output the usage to standard out.
 */
static void
output_help()
{
    printf("Usage: %s [option...] sub-command\n", program_name);
    printf("Options:\n");
    printf("  -c FILE  --configuration-file FILE\n");
    printf("                             specify a configuration file\n");
    printf("                             (default: %s)\n",
	DEFAULT_CONFIGURATION_FILE_NAME);
    printf("  -h  --help                 display this help, then exit\n");
    printf("  -v  --version              display version number, then exit\n");
    printf("\nSub-commands:\n");
    printf("  kill                       kill ndtpd (send SIGKILL)\n");
    printf("  restart                    restart ndtpd (send SIGHUP)\n");
    printf("  status                     output status of ndtpd\n");
    printf("  terminate                  terminate ndtpd (send SIGTERM)\n");
    printf("\nDefault value used in a configuration file:\n");
    printf("  work-path                  %s\n", DEFAULT_WORK_PATH);
    printf("\nReport bugs to %s.\n", MAILING_ADDRESS);
    fflush(stdout);
}



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
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, unistd.h)
 *   AC_CHECK_FUNCS(strchr)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
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

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#include "readconf.h"

#ifdef __STDC__
static Configuration *find_directive(Configuration *, const char *,
    const char *);
static void reset_configuration(Configuration *, const char *);
static Configuration *check_configuration(Configuration *, const char *);
#else /* not __STDC__ */
static Configuration *find_directive();
static void reset_configuration();
static Configuration *check_configuration();
#endif /* not __STDC__ */

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

#define READCONF_IS_REDEFINABLE(t) \
((t) == READCONF_ZERO_OR_MORE || (t) == READCONF_ONCE_OR_MORE)

#define READCONF_IS_REQUIRED(t) \
((t) == READCONF_ONCE || (t) == READCONF_ONCE_OR_MORE)


/*
 * Read configurations from the file `file_name'.
 * `table' is a table which lists all recognized directives.
 * If no error occurrs, 0 is returned.  Otherwise -1 is returned.
 */
int
read_configuration(file_name, configuration_table)
    const char *file_name;
    Configuration *configuration_table;
{
    Configuration *directive;
    Configuration *lack_directive;
    FILE *file;
    char line_buffer[READCONF_SIZE_LINE + 1];
    char block[READCONF_SIZE_BLOCK_NAME + 1];
    char *space;
    char *name;
    char *argument;
    int line_number = 0;
    char *p;


    /*
     * Reset counters in the directive table.
     */
    block[0] = '\0';
    reset_configuration(configuration_table, block);

    /*
     * Call a pre-process hook.
     */
    directive = find_directive(configuration_table, block, "begin");
    if (directive != NULL
	&& directive->function(NULL, file_name, line_number) < 0)
	return -1;

    /*
     * Open the configuration file.
     */
    file = fopen(file_name, "r");
    if (file == NULL) {
	syslog(LOG_ERR, "%s: failed to open the file, %s", file_name,
	    strerror(errno));
	goto failed;
    }
    syslog(LOG_DEBUG, "%s: debug: open the file", file_name);

    /*
     * Read each line in the configuration file.
     */
    while (fgets(line_buffer, READCONF_SIZE_LINE + 1, file) != NULL) {
	line_number++;

	/* 
	 * Check for the line length.
	 * An error occurs if it exceeds a limit.
	 */
	p = strchr(line_buffer, '\n');
	if (p != NULL)
	    *p = '\0';
	else if (strlen(line_buffer) == READCONF_SIZE_LINE) {
	    *(line_buffer + READCONF_SIZE_LINE) = '\0';
	    syslog(LOG_ERR, "%s:%d: too long line: %s...", file_name,
		line_number, line_buffer);
	    goto failed;
	}

	/*
	 * Get a directive name.
	 */
	for (name = line_buffer; *name == ' ' || *name == '\t'; name++)
	    ;

	/*
	 * Ignore this line if the line is empty or starts with '#'.
	 */
	if (*name == '#') {
	    syslog(LOG_DEBUG, "%s:%d: debug: comment line", file_name,
		line_number);
	    continue;
	}
	if (*name == '\0') {
	    syslog(LOG_DEBUG, "%s:%d: debug: empty line", file_name,
		line_number);
	    continue;
	}

	/*
	 * Get an argument to the directive name.
	 */
	for (p = name; *p != ' ' && *p != '\t' && *p != '\0'; p++)
	    ;
	if (*p != '\0') {
	    *p++ = '\0';
	    while (*p == ' ' || *p == '\t')
		p++;
	}
	if (*p == '\0')
	    argument = NULL;
	else
	    argument = p;

	/*
	 * Delete spaces and tabls in the tail of the line.
	 */
	if (argument != NULL) {
	    for (p = argument, space = NULL; *p != '\0'; p++) {
		if (*p == ' ' || *p == '\t') {
		    if (space == NULL)
			space = p;
		} else
		    space = NULL;
	    }
	    if (space != NULL)
		*space = '\0';
	}

	/*
	 * Check for the directive name.
	 */
	if (strcmp(name, "begin") == 0) {
	    char argument_begin[(READCONF_SIZE_BLOCK_NAME * 2) + 1];

	    /*
	     * `begin' keyword.  Beginning of the block.
	     * Check for the argument.
	     */
	    if (argument == NULL) {
		syslog(LOG_ERR, "%s:%d: missing argument: begin", file_name,
		    line_number);
		goto failed;
	    }
	    if (READCONF_SIZE_BLOCK_NAME
		< strlen(block) + 1 + strlen(argument)) {
		syslog(LOG_ERR, "%s:%d: unkonwn directive name: begin %s",
		    file_name, line_number, argument);
		goto failed;
	    }

	    /*
	     * Find a directive entry in the table.
	     */
	    strcpy(argument_begin, "begin ");
	    strcat(argument_begin, argument);
	    directive = find_directive(configuration_table, block,
		argument_begin);
	    if (directive == NULL) {
		syslog(LOG_ERR, "%s:%d: unknown directive name: begin %s",
		    file_name, line_number, argument);
		goto failed;
	    }
	    directive->count++;
	    if (1 < directive->count
		&& !READCONF_IS_REDEFINABLE(directive->type)) {
		syslog(LOG_ERR, "%s:%d: directive redefined: begin %s",
		    file_name, line_number, argument);
		goto failed;
	    }

	    /*
	     * Update the current block name.
	     */
	    if (block[0] != '\0')
		strcat(block, " ");
	    strcat(block, argument);

	    /*
	     * Reset counters for the block.
	     */
	    reset_configuration(configuration_table, block);

	    /*
	     * Dispatch.
	     */
	    if (directive->function != NULL
		&& directive->function(NULL, file_name, line_number) < 0)
		goto failed;

	} else if (strcmp(name, "end") == 0) {
	    char argument_end[(READCONF_SIZE_BLOCK_NAME * 2) + 1];

	    /*
	     * `end' keyword.  End of the book definition.
	     * Check for the argument.
	     */
	    if (argument != NULL) {
		syslog(LOG_ERR, "%s:%d: unwanted argument: %s", file_name,
		    line_number, argument);
		goto failed;
	    }

	    /*
	     * If there is no correspondig `begin' keyword, an error is
	     * caused.
	     */
	    if (block[0] == '\0') {
		syslog(LOG_ERR, "%s:%d: unexpected: end", file_name,
		    line_number);
		goto failed;
	    }

	    /*
	     * Checks for counters.
	     * If required directive is not defined, an error occurs.
	     */
	    lack_directive = check_configuration(configuration_table, block);
	    if (lack_directive != NULL) {
		syslog(LOG_ERR, "%s:%d: missing directive: %s", file_name,
		    line_number, lack_directive->name);
		goto failed;
	    }

	    /*
	     * Update the current block name, and get a directive
	     * name to be found by `find_directive()'.
	     */
	    p = strrchr(block, ' ');
	    strcpy(argument_end, "end ");
	    if (p != NULL) {
		strcat(argument_end, p + 1);
		*p = '\0';
	    } else {
		strcat(argument_end, block);
		block[0] = '\0';
	    }

	    /*
	     * Dispatch.
	     */
	    directive = find_directive(configuration_table, block,
		argument_end);
	    if (directive == NULL) {
		syslog(LOG_ERR, "%s:%d: unexpected: end", file_name,
		    line_number);
		goto failed;
	    }
	    if (directive->function != NULL
		&& directive->function(NULL, file_name, line_number) < 0)
		goto failed;

        } else {
	    /*
	     * It is a directive.
	     * Look for a directive entry in the table.
	     */
	    if (argument == NULL) {
		syslog(LOG_ERR, "%s:%d: missing argument: %s", file_name,
		    line_number, name);
		goto failed;
	    }
	    directive = find_directive(configuration_table, block, name);
	    if (directive == NULL) {
		syslog(LOG_ERR, "%s:%d: unknown directive: %s", file_name,
		    line_number, name);
		goto failed;
	    }
	    directive->count++;
	    if (1 < directive->count
		&& !READCONF_IS_REDEFINABLE(directive->type)) {
		syslog(LOG_ERR, "%s:%d: directive redefined: %s", file_name,
		    line_number, name);
		goto failed;
	    }

	    /*
	     * Dispatch.
	     */
	    if (directive->function != NULL
		&& directive->function(argument, file_name, line_number) < 0)
		goto failed;
	}
    }

    /*
     * End of the configuration file.
     * Check for balance of "begin" and "end".
     */
    if (block[0] != '\0') {
	syslog(LOG_ERR, "%s: missing: end", file_name, line_number);
	goto failed;
    }

    /*
     * Checks for counters in the directive table.
     * If required directive is not defined, an error occurs.
     */
    lack_directive = check_configuration(configuration_table, block);
    if (lack_directive != NULL) {
	syslog(LOG_ERR, "%s: missing directive: %s", file_name,
	    lack_directive->name);
	    goto failed;
    }

    /*
     * Call a post-process hook.
     */
    directive = find_directive(configuration_table, block, "end");
    if (directive != NULL
	&& directive->function(NULL, file_name, line_number) < 0)
	goto failed;

    /*
     * Close the configuration file.
     */
    if (fclose(file) == EOF) {
	syslog(LOG_ERR, "%s: failed to close the file, %s", file_name,
	    strerror(errno));
	file = NULL;
	goto failed;
    }
    syslog(LOG_DEBUG, "%s: debug: close the file", file_name);

    return 0;
    
    /*
     * An error occurs...
     */
  failed:
    if (file != NULL && fclose(file) == EOF)
	syslog(LOG_ERR, "%s: failed to close the file, %s", file_name,
	    strerror(errno));
    return -1;
}


/*
 * Find the directive which has `block' and `name' pair.
 *
 * If found, the pointer to the entry is returned.  Otherwise NULL is
 * returned.
 */
static Configuration *
find_directive(configuration_table, block, name)
    Configuration *configuration_table;
    const char *block;
    const char *name;
{
    Configuration *directive;

    for (directive = configuration_table; directive->name != NULL;
	 directive++) {
	if (strcmp(directive->name, name) == 0
	    && strcmp(directive->block, block) == 0)
	    return directive;
    }
    return NULL;
}


/*
 * Reset counters which have `block' name in the table.
 */
static void
reset_configuration(configuration_table, block)
    Configuration *configuration_table;
    const char *block;
{
    Configuration *directive;

    for (directive = configuration_table; directive->name != NULL;
	 directive++) {
	if (directive->block != NULL && strcmp(directive->block, block) == 0)
	    directive->count = 0;
    }
}


/*
 * Check for counters in the configuration table.
 *
 * If required directive is not defined, its name is returned.
 * When more than one required table are undefined, the ealiest one in
 * the table is returned.
 *
 * If all required table are defined, NULL is returned.
 */
static Configuration *
check_configuration(configuration_table, block)
    Configuration *configuration_table;
    const char *block;
{
    Configuration *directive;

    for (directive = configuration_table; directive->name != NULL;
	 directive++) {
	if (directive->block != NULL
	    && strcmp(directive->block, block) == 0
	    && directive->count == 0
	    && READCONF_IS_REQUIRED(directive->type))
	    return directive;
    }
    return NULL;
}



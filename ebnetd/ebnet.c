/*
 * Copyright) 2003
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
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* not HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* not HAVE_DIRENT_H */

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif

#ifndef HAVE_STRTOL
#ifdef __STDC__
long strtol(const char *, char **, int);
#else /* not __STDC__ */
long strtol();
#endif /* not __STDC__ */
#endif /* not HAVE_STRTOL */

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

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

#include "defs.h"
#include "fakelog.h"
#include "ioall.h"
#include "linebuf.h"
#include "ticket.h"

#include "ebtiny/eb.h"
#include "ebtiny/error.h"

/*
 * Maximum length of a line.
 */
#define EBNET_MAX_LINE_LENGTH		511

/*
 * The maximum number of arguments in a line.
 */
#define EBNET_MAX_LINE_ARGUMENTS	5

/*
 * The maximum length of the <path> argument.
 */
#define EBNET_MAX_PATH_LENGTH		33

/*
 * The maximum length of the <length> argument.
 */
#define EBNET_MAX_READ_LENGTH		65536

/*
 * Suffix for appendix.
 */
#define EBNET_APPENDIX_SUFFIX		"app"


/*
 * Error codes.
 */
typedef int EBNet_Error_Code;

#define EBNET_ERROR_OK			 0
#define EBNET_ERROR_FAIL_OPEN_FILE	 1
#define EBNET_ERROR_FAIL_SEEK_FILE	 2
#define EBNET_ERROR_FAIL_READ_FILE	 3
#define EBNET_ERROR_BAD_BOOK_NAME	 4

#define EBNET_ERROR_BAD_READ_LENGTH	 5
#define EBNET_ERROR_BAD_READ_OFFSET	 6
#define EBNET_ERROR_BAD_PATH		 7
#define EBNET_ERROR_BAD_SUBBOOK_NAME	 8
#define EBNET_ERROR_NO_APPENDIX		10

#define EBNET_ERROR_BUSY		11
#define EBNET_ERROR_NO_PERMISSION	12

/*
 * Error response categories for the error codes.
 */
static const char *error_categories[] = {
    "OK",
    "ERROR",
    "ERROR",
    "ERROR",
    "ERROR",

    "ERROR",
    "ERROR",
    "ERROR",
    "ERROR",
    "ERROR",

    "BUSY",
    "DENY"
};

/*
 * Error descriptions for the error codes.
 */
static const char *error_messages[] = {
    "accepted",
    "failed to open the file",
    "failed to seek the file",
    "failed to read the file",
    "invalid book name",

    "invalid read length",
    "invalid read offset",
    "invalid file or directory name",
    "invalid subbook name",
    "the book doesn't have appendix",

    "full of clients",
    "permission denied"
};

/*
 * Book name type -- book or appendix.
 */
typedef int EBNet_Content_Type;

#define EBNET_CONTENT_TYPE_BOOK		0
#define EBNET_CONTENT_TYPE_APPENDIX	1

/*
 * Pointer to data buffer for the READ command.
 *
 * For efficiency, we prepare buffer for the READ command statically.
 * Memory allocation to `read_buffer' is done by protocol_main().
 */
static char *read_buffer = NULL;

/*
 * File descriptor cache for the READ command.
 */
static int cache_file = -1;
static char cache_book_name[MAX_BOOK_NAME_LENGTH + 4];  /* +4 for ".app" */
static char cache_file_path[EBNET_MAX_PATH_LENGTH + 1];

/*
 * Unexported functions.
 */
static void split_line EBNETD_P((char *, int, int *, char *[]));
static int command_booklist EBNETD_P((int, char *[]));
static int command_book EBNETD_P((int, char *[]));
static int command_dir EBNETD_P((int, char *[]));
static int command_file EBNETD_P((int, char *[]));
static int command_filesize EBNETD_P((int, char *[]));
static int command_read EBNETD_P((int, char *[]));

static EBNet_Error_Code parse_and_check_book_name EBNETD_P((const char *,
    Book **, EBNet_Content_Type *));
static EBNet_Error_Code check_file_path EBNETD_P((Book *, EBNet_Content_Type,
    const char *));
static EBNet_Error_Code check_directory_path EBNETD_P((Book *,
    EBNet_Content_Type, const char *));
static EBNet_Error_Code check_path_parent_directory EBNETD_P((const char *));
static EBNet_Error_Code check_subbook_name EBNETD_P((Book *,
    EBNet_Content_Type, const char *));
static EBNet_Error_Code check_root_file_name EBNETD_P((const char *));

static void truncate_string EBNETD_P((char *, size_t));

/*
 * EBNet command table.
 * Note that the "QUIT" command is not listed here.
 */
typedef struct {
    char *name;
    int (*function) EBNETD_P((int, char *[]));
    int argument_count;
} EBNet_Command;

static const EBNet_Command ebnet_commands[] = {
    {"BOOKLIST", command_booklist,  1},
    {"BOOK",     command_book,      2},
    {"DIR",      command_dir,       4},
    {"FILE",     command_file,      4},
    {"FILESIZE", command_filesize,  3},
    {"READ",     command_read,      5},
    {NULL,       NULL,              0}
};


/*
 * Communicate with a client by EBNet.
 */
int
protocol_main()
{
    Line_Buffer line_buffer;
    char line[EBNET_MAX_LINE_LENGTH + 1];
    ssize_t line_length;
    char *arguments[EBNET_MAX_LINE_ARGUMENTS + 2];
    int argument_count;
    const EBNet_Command *command;

    initialize_line_buffer(&line_buffer);
    bind_file_to_line_buffer(&line_buffer, accepted_in_file);
    set_line_buffer_timeout(&line_buffer, idle_timeout);

    /*
     * Initialize `read_buffer'.
     */
    read_buffer = malloc(EBNET_MAX_READ_LENGTH);
    if (read_buffer == NULL) {
	syslog(LOG_ERR, "memory exhausted");
	goto die;
    }

    for (;;) {
	/*
	 * Read a line received from the client.
	 */
	line_length = read_line_buffer(&line_buffer, line,
	    EBNET_MAX_LINE_LENGTH);
	if (line_length < 0)
	    goto die;
	else if (line_length == 0)
	    continue;

	/*
	 * Check for the line length.
	 */
	if (EBNET_MAX_LINE_LENGTH <= line_length) {
	    if (skip_line_buffer(&line_buffer) < 0)
		goto die;
            *(line + 30) = '\0';
	    syslog(LOG_INFO, "too long request line: line=%s..., host=%s(%s)",
		line, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout,
		"!ERROR; too long request line\r\n") <= 0) {
		goto die;
	    }
	    continue;
	}
	syslog(LOG_DEBUG, "debug: received line: %s", line);

	/*
	 * Split the request line into an array of arguments.
	 */
	split_line(line, EBNET_MAX_LINE_ARGUMENTS + 1, &argument_count,
	    arguments);
	if (argument_count == 0)
	    continue;

	if (strcasecmp(arguments[0], "QUIT") == 0)
	    break;

        /*
         * Scan the command table.
         */
        for (command = ebnet_commands; command->name != NULL; command++) {
	    if (strcasecmp(command->name, arguments[0]) == 0)
		break;
	}
	if (command->name == NULL) {
	    syslog(LOG_INFO, "invalid command: cmd=%s, host=%s(%s)",
		arguments[0], client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout,
		"!ERROR; invalid command\r\n") <= 0) {
		syslog(LOG_INFO, "failed to send message: host=%s(%s)", 
		    client_host_name, client_address);
		goto die;
	    }
	    continue;
	}

	/*
	 * Check the number of arguments to the command.
	 */
	if (argument_count != command->argument_count) {
	    syslog(LOG_INFO,
		"the wrong number of arguments: cmd=%s, host=%s(%s)",
		command->name, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout,
		"!ERROR; the wrong number of arguments\r\n") <= 0) {
		syslog(LOG_INFO, "failed to send message: host=%s(%s)", 
		    client_host_name, client_address);
		goto die;
	    }
	    continue;
	}

	/*
	 * Dispatch.
	 */
	if ((command->function)(argument_count, arguments) < 0) {
	    syslog(LOG_INFO, "failed to send message: host=%s(%s)", 
		client_host_name, client_address);
	    goto die;
	}
    }

    free(read_buffer);
    read_buffer = NULL;
    finalize_line_buffer(&line_buffer);
    return 0;

  die:
    if (read_buffer != NULL) {
	free(read_buffer);
	read_buffer = NULL;
    }
    if (0 <= cache_file) {
	close(cache_file);
	cache_file = -1;
    }
    finalize_line_buffer(&line_buffer);
    return -1;
}


/*
 * Split a request line into an array of arguments.
 *
 * Each argument is separated by a tab or a space.  `line' is a request
 * line.  The function splits no more than `max_argument_count' arguments.
 * The result is put into `arguments[]' as an array of arguments, and the
 * length of the array is put into `*argument_count'.
 */
static void
split_line(line, max_argument_count, argument_count, arguments)
    char *line;
    int max_argument_count;
    int *argument_count;
    char *arguments[];
{
    char *p = line;
    int i;

    i = 0;
    for (;;) {
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p == '\0' || max_argument_count <= i)
	    break;
	arguments[i++] = p++;
	while (*p != ' ' && *p != '\t' && *p != '\0')
	    p++;
	if (*p != '\0')
	    *p++ = '\0';
    }

    arguments[i] = NULL;
    *argument_count = i;
}


/*
 * Perform the BOOKLIST command.
 *
 * Syntax:
 *   BOOKLIST
 */
static int
command_booklist(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    char message[MAX_BOOK_NAME_LENGTH + MAX_BOOK_TITLE_LENGTH + 8];

    syslog(LOG_INFO, "%s: cmd=BOOKLIST, host=%s(%s)",
	"ok", client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; book names and an empty line follows\r\n") <= 0)
	return -1;

    for (book = book_registry; book != NULL; book = book->next) {
	if (!book->permission_flag)
	    continue;
	sprintf(message, "%s %s\r\n", book->name, book->title);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	    return -1;
	if (book->appendix_path != NULL) {
	    sprintf(message, "%s.app %s\r\n", book->name, book->title);
	    if (write_string_all(accepted_out_file, idle_timeout, message)
		<= 0) {
		return -1;
	    }
	}
    }

    if (write_string_all(accepted_out_file, idle_timeout, "\r\n") <= 0)
	return -1;

    return 0;
}


/*
 * Perform the BOOK command.
 *
 * Syntax:
 *   BOOK <book>
 */
static int
command_book(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    EBNet_Content_Type content_type;
    char message[EBNET_MAX_LINE_LENGTH + 1];
    EBNet_Error_Code error_code;

    /*
     * Parse and check arguments.
     */
    error_code = parse_and_check_book_name(arguments[1], &book, &content_type);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    /*
     * Get a ticket to access the book.
     */
    if (book->max_clients != 0 && get_ticket(&book->ticket_stock) < 0) {
	error_code = EBNET_ERROR_BUSY;
	goto error;
    }
	
    /*
     * Send a response message.
     */
    syslog(LOG_INFO, "%s: cmd=BOOK, book=%s, host=%s(%s)",
	"ok", arguments[1], client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; accepted\r\n") <= 0)
	return -1;
    return 0;

    /*
     * An error occurs...
     */
  error:
    /* +4 for ".app" */
    truncate_string(arguments[1], MAX_BOOK_NAME_LENGTH + 4);
    syslog(LOG_INFO, "%s: cmd=BOOK, book=%s, host=%s(%s)",
	error_messages[error_code], arguments[1],
	client_host_name, client_address);

    sprintf(message, "!%s; %s\r\n", error_categories[error_code],
	error_messages[error_code]);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    return 0;
}


/*
 * Perform the DIR command.
 *
 * Syntax:
 *   DIR <book> <path> <directory-name>
 */
static int
command_dir(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    char absolute_path[PATH_MAX + 1];
    EBNet_Content_Type content_type;
    const char *prefix;
    char actual_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char message[EBNET_MAX_LINE_LENGTH + 1];
    EBNet_Error_Code error_code;

    /*
     * Parse and check arguments.
     */
    error_code = parse_and_check_book_name(arguments[1], &book, &content_type);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    error_code = check_directory_path(book, content_type, arguments[2]);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    if (EB_MAX_DIRECTORY_NAME_LENGTH < strlen(arguments[3])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    if (strcmp(arguments[2], "/") == 0) {
	error_code = check_subbook_name(book, content_type, arguments[3]);
	if (error_code != EBNET_ERROR_OK)
	    goto error;
    } else {
	error_code = check_path_parent_directory(arguments[3]);
	if (error_code != EBNET_ERROR_OK)
	    goto error;
    }

    /*
     * Get an actual directory name. 
     */
    if (content_type == EBNET_CONTENT_TYPE_BOOK)
	prefix = book->path;
    else
	prefix = book->appendix_path;

    if (PATH_MAX < strlen(prefix) + 1 + strlen(arguments[2])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    sprintf(absolute_path, "%s%s", prefix, arguments[2]);

    strcpy(actual_directory_name, arguments[3]);
    if (eb_fix_directory_name(absolute_path, actual_directory_name)
	!= EB_SUCCESS) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }

    /*
     * Send a response message.
     */
    syslog(LOG_INFO, "%s: cmd=DIR, book=%s, path=%s, dir=%s, host=%s(%s)",
	"ok", arguments[1], arguments[2], arguments[3],
	client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; actual directory name follows\r\n") <= 0)
	return -1;
    if (write_string_all(accepted_out_file, idle_timeout,
	actual_directory_name) <= 0)
	return -1;
    if (write_string_all(accepted_out_file, idle_timeout, "\r\n") <= 0)
	return -1;
    return 0;

    /*
     * An error occurs...
     */
  error:
    /* +4 for ".app" */
    truncate_string(arguments[1], MAX_BOOK_NAME_LENGTH + 4);
    truncate_string(arguments[2], EBNET_MAX_PATH_LENGTH);
    truncate_string(arguments[3], EB_MAX_DIRECTORY_NAME_LENGTH);
    syslog(LOG_INFO, "%s: cmd=DIR, book=%s, path=%s, dir=%s, host=%s(%s)",
	error_messages[error_code], arguments[1], arguments[2], arguments[3],
	client_host_name, client_address);

    sprintf(message, "!%s; %s\r\n", error_categories[error_code],
	error_messages[error_code]);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    return 0;
}


/*
 * Perform the FILE command.
 *
 * Syntax:
 *   FILE <book> <path> <file-name>
 */
static int
command_file(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    char absolute_path[PATH_MAX + 1];
    EBNet_Content_Type content_type;
    const char *prefix;
    char actual_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char message[EBNET_MAX_LINE_LENGTH + 1];
    EBNet_Error_Code error_code;

    /*
     * Check arguments.
     */
    error_code = parse_and_check_book_name(arguments[1], &book, &content_type);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    error_code = check_directory_path(book, content_type, arguments[2]);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    if (EB_MAX_FILE_NAME_LENGTH < strlen(arguments[3])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    if (strcmp(arguments[2], "/") == 0) {
	error_code = check_root_file_name(arguments[3]);
	if (error_code != EBNET_ERROR_OK)
	    goto error;
    } else {
	error_code = check_path_parent_directory(arguments[3]);
	if (error_code != EBNET_ERROR_OK)
	    goto error;
    }

    /*
     * Get an actual file name. 
     */
    if (content_type == EBNET_CONTENT_TYPE_BOOK)
	prefix = book->path;
    else
	prefix = book->appendix_path;

    if (PATH_MAX < strlen(prefix) + 1 + strlen(arguments[2])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    sprintf(absolute_path, "%s%s", prefix, arguments[2]);

    if (eb_find_file_name(absolute_path, arguments[3], actual_file_name)
	!= EB_SUCCESS) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }

    /*
     * Send a response message.
     */
    syslog(LOG_INFO, "%s: cmd=FILE, book=%s, path=%s, file=%s, host=%s(%s)",
	"ok", arguments[1], arguments[2], arguments[3],
	client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; actual file name follows\r\n") <= 0)
	return -1;
    if (write_string_all(accepted_out_file, idle_timeout,
	actual_file_name) <= 0)
	return -1;
    if (write_string_all(accepted_out_file, idle_timeout, "\r\n") <= 0)
	return -1;
    return 0;

    /*
     * An error occurs...
     */
  error:
    /* +4 for ".app" */
    truncate_string(arguments[1], MAX_BOOK_NAME_LENGTH + 4);
    truncate_string(arguments[2], EBNET_MAX_PATH_LENGTH);
    truncate_string(arguments[3], EB_MAX_FILE_NAME_LENGTH);
    syslog(LOG_INFO, "%s: cmd=FILE, book=%s, path=%s, file=%s, host=%s(%s)",
	error_messages[error_code], arguments[1], arguments[2], arguments[3],
	client_host_name, client_address);

    sprintf(message, "!%s; %s\r\n", error_categories[error_code],
	error_messages[error_code]);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    return 0;
}


/*
 * Perform the FILESIZE command.
 *
 * Syntax:
 *   FILESIZE <book> <path>
 */
static int
command_filesize(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    char absolute_path[PATH_MAX + 1];
    EBNet_Content_Type content_type;
    const char *prefix;
    char message[EBNET_MAX_LINE_LENGTH + 1];
    EBNet_Error_Code error_code;
    struct stat st;

    /*
     * Check arguments.
     */
    error_code = parse_and_check_book_name(arguments[1], &book, &content_type);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    error_code = check_file_path(book, content_type, arguments[2]);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    /*
     * Get an actual file name. 
     */
    if (content_type == EBNET_CONTENT_TYPE_BOOK)
	prefix = book->path;
    else
	prefix = book->appendix_path;

    if (PATH_MAX < strlen(prefix) + 1 + strlen(arguments[2])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    sprintf(absolute_path, "%s%s", prefix, arguments[2]);

    /*
     * Get file size.
     */
    if (stat(absolute_path, &st) < 0 || (st.st_mode & S_IFMT) != S_IFREG) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }

    /*
     * Send a response message.
     */
    syslog(LOG_INFO, "%s: cmd=FILESIZE, book=%s, path=%s, host=%s(%s)",
	"ok", arguments[1], arguments[2], client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; file size follows\r\n") <= 0)
	return -1;
    sprintf(message, "%ld\r\n", (long)st.st_size);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    
    return 0;

    /*
     * An error occurs...
     */
  error:
    /* +4 for ".app" */
    truncate_string(arguments[1], MAX_BOOK_NAME_LENGTH + 4);
    truncate_string(arguments[2], EBNET_MAX_PATH_LENGTH);
    syslog(LOG_INFO, "%s: cmd=FILESIZE, book=%s, path=%s, host=%s(%s)",
	error_messages[error_code], arguments[1], arguments[2],
	client_host_name, client_address);

    sprintf(message, "!%s; %s\r\n", error_categories[error_code],
	error_messages[error_code]);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    return 0;
}


/*
 * Perform the READ command.
 *
 * Syntax:
 *   READ <book> <path> <offset> <length>
 */
static int
command_read(argument_count, arguments)
    int argument_count;
    char *arguments[];
{
    Book *book;
    char absolute_path[PATH_MAX + 1];
    EBNet_Content_Type content_type;
    const char *prefix;
    char message[EBNET_MAX_LINE_LENGTH + 1];
    EBNet_Error_Code error_code;
    off_t offset;
    ssize_t data_length;
    char *end_p;
    int file = -1;
    size_t total_sent_length;

    /*
     * Check arguments.
     */
    error_code = parse_and_check_book_name(arguments[1], &book, &content_type);
    if (error_code != EBNET_ERROR_OK)
	goto error;
    if (book->max_clients != 0 && get_ticket(&book->ticket_stock) < 0) {
	error_code = EBNET_ERROR_BUSY;
	goto error;
    }

    error_code = check_file_path(book, content_type, arguments[2]);
    if (error_code != EBNET_ERROR_OK)
	goto error;

    offset = (off_t)strtol(arguments[3], &end_p, 10);
    if (!isdigit(*(arguments[3])) || *end_p != '\0' || offset < 0) {
	error_code = EBNET_ERROR_BAD_READ_OFFSET;
	goto error;
    }

    data_length = (off_t)strtol(arguments[4], &end_p, 10);
    if (!isdigit(*(arguments[4])) || *end_p != '\0' || data_length <= 0) {
	error_code = EBNET_ERROR_BAD_READ_LENGTH;
	goto error;
    }

    /*
     * Get an actual file name. 
     */
    if (content_type == EBNET_CONTENT_TYPE_BOOK)
	prefix = book->path;
    else
	prefix = book->appendix_path;

    if (PATH_MAX < strlen(prefix) + 1 + strlen(arguments[2])) {
	error_code = EBNET_ERROR_BAD_PATH;
	goto error;
    }
    sprintf(absolute_path, "%s%s", prefix, arguments[2]);

    /*
     * Open and seek the file.
     */
    if (0 <= cache_file
	&& strcmp(arguments[1], cache_book_name) == 0
	&& strcmp(arguments[2], cache_file_path) == 0) {
	file = cache_file;
    } else {
	file = open(absolute_path, O_RDONLY);
	if (file < 0) {
	    error_code = EBNET_ERROR_FAIL_OPEN_FILE;
	    goto error;
	}
	if (0 <= cache_file)
	    close(cache_file);
	cache_file = file;
	strcpy(cache_book_name, arguments[1]);
	strcpy(cache_file_path, arguments[2]);
    }
    if (lseek(file, offset, SEEK_SET) < 0) {
	error_code = EBNET_ERROR_FAIL_SEEK_FILE;
	goto error;
    }

    /*
     * Send a response message.
     */
    syslog(LOG_INFO,
	"%s: cmd=READ, book=%s, file=%s, offset=%ld, length=%ld, host=%s(%s)",
	"ok", arguments[1], arguments[2], (long)offset, (long)data_length,
	client_host_name, client_address);

    if (write_string_all(accepted_out_file, idle_timeout,
	"!OK; read data follows\r\n") <= 0) {
	close(file);
	cache_file = -1;
	return -1;
    }

    total_sent_length = 0;
    while (total_sent_length < data_length) {
	/*
	 * Read data from the file.
	 */
	ssize_t read_result;
	ssize_t n;

	if (data_length - total_sent_length < EBNET_MAX_READ_LENGTH)
	    n = data_length - total_sent_length;
	else
	    n = EBNET_MAX_READ_LENGTH;

	errno = 0;
	read_result = read(file, read_buffer, n);
	if (read_result < 0) {
	    if (errno == EINTR)
		continue;
	    if (write_string_all(accepted_out_file, idle_timeout,
		"*-1\r\n") <= 0) {
		close(file);
		cache_file = -1;
		return -1;
	    }
	    break;
	} else if (read_result == 0) {
	    if (write_string_all(accepted_out_file, idle_timeout,
		"*0\r\n") <= 0) {
		close(file);
		cache_file = -1;
		return -1;
	    }
	    break;
	}

	/*
	 * Send data to client.
	 */
	sprintf(message, "*%ld\r\n", (long)read_result);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0) {
	    close(file);
	    cache_file = -1;
	    return -1;
	}
	if (write_all(accepted_out_file, idle_timeout, read_buffer,
	    read_result) <= 0) {
	    close(file);
	    cache_file = -1;
	    return -1;
	}
	total_sent_length += read_result;
    }

    return 0;

    /*
     * An error occurs...
     */
  error:
    if (0 <= file) {
	if (file == cache_file)
	    cache_file = -1;
	close(file);
    }

    /* +4 for ".app" */
    truncate_string(arguments[1], MAX_BOOK_NAME_LENGTH + 4);
    truncate_string(arguments[2], EBNET_MAX_PATH_LENGTH);
    truncate_string(arguments[3], 10);
    truncate_string(arguments[4], 10);
    syslog(LOG_INFO, 
	"%s: cmd=READ, book=%s, file=%s, offset=%s, length=%s, host=%s(%s)",
	error_messages[error_code], arguments[1], arguments[2],
	arguments[3], arguments[4], client_host_name, client_address);

    sprintf(message, "!%s; %s\r\n", error_categories[error_code],
	error_messages[error_code]);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;
    return 0;
}


/*
 * Parse and check a book name.
 * Book name must be either form:
 *
 *    BOOK_NAME              -- book
 *    BOOK_NAME ".app"       -- appendix
 *
 * If a book name given as `name' is valid, then `Book' information of
 * BOOK_NAME is put into `*book' and its content type (book or appendix)
 * into `content_type'.  Otherwise `*book' is set to NULL.
 *
 * Upon success, this function returns EB_ERROR_OK, otherwise it returns
 * an error code.
 */
static EBNet_Error_Code
parse_and_check_book_name(name, book, content_type)
    const char *name;
    Book **book;
    EBNet_Content_Type *content_type;
{
    const char *dot;
    char book_name[MAX_BOOK_NAME_LENGTH + 1];
    size_t book_name_length;
    Book *book_temporary;
    
    *book = NULL;

    /*
     * Parse a book name.
     */
    dot = strchr(name, '.');
    if (dot == NULL)
	book_name_length = strlen(name);
    else
	book_name_length = dot - name;

    if (book_name_length == 0 || MAX_BOOK_NAME_LENGTH < book_name_length)
	return EBNET_ERROR_BAD_BOOK_NAME;
    memcpy(book_name, name, book_name_length);
    *(book_name + book_name_length) = '\0';

    if (dot == NULL)
	*content_type = EBNET_CONTENT_TYPE_BOOK;
    else if (strcmp(dot + 1, EBNET_APPENDIX_SUFFIX) == 0)
	*content_type = EBNET_CONTENT_TYPE_APPENDIX;
    else
	return EBNET_ERROR_BAD_BOOK_NAME;

    /*
     * Check the book name.
     */
    book_temporary = find_book(book_name);
    if (book_temporary == NULL)
	return EBNET_ERROR_BAD_BOOK_NAME;
    if (!book_temporary->permission_flag)
	return EBNET_ERROR_NO_PERMISSION;
    if (*content_type == EBNET_CONTENT_TYPE_APPENDIX
	&& book_temporary->appendix_path == NULL)
	return EBNET_ERROR_NO_APPENDIX;

    *book = book_temporary;
    return EBNET_ERROR_OK;
}


/*
 * Check a file-path arugment in a request line.
 * Book name must be either form:
 *
 *    "/" SUBBOOK_NAME *( "/" DIRECTORY_NAME ) "/" FILE_NAME
 *    "/" ROOT_FILE_NAME
 *
 * `book' and `content_type' is the parse result of book name in the
 * request, set by parse_and_check_book_name().
 *
 * If the arguments are valid, the function returns EB_ERROR_OK,
 * otherwise it returns an error code.
 */
static EBNet_Error_Code
check_file_path(book, content_type, path)
    Book *book;
    EBNet_Content_Type content_type;
    const char *path;
{
    char subbook_name[EB_MAX_FILE_NAME_LENGTH + 1];
    size_t subbook_name_length;
    EBNet_Error_Code error_code;
    char *slash;

    if (*path != '/')
	return EBNET_ERROR_BAD_PATH;

    slash = strchr(path + 1, '/');
    if (slash == NULL) {
	/*
	 * Check root file name.
	 */
	error_code = check_root_file_name(path + 1);
	if (error_code != EBNET_ERROR_OK)
	    return error_code;
    } else {
	/*
	 * Get a subbook name and check whether `book' has a subbook
	 * with that name.
	 */
	subbook_name_length = slash - (path + 1);
	if (subbook_name_length == 0
	    || EB_MAX_DIRECTORY_NAME_LENGTH < subbook_name_length)
	    return EBNET_ERROR_BAD_SUBBOOK_NAME;
	memcpy(subbook_name, path + 1, subbook_name_length);
	*(subbook_name + subbook_name_length) = '\0';

	error_code = check_subbook_name(book, content_type, subbook_name);
	if (error_code != EBNET_ERROR_OK)
	    return error_code;

	/*
	 * Check rest of the path.
	 */
	error_code = check_path_parent_directory(slash);
	if (error_code != EBNET_ERROR_OK)
	    return error_code;
    }

    return EBNET_ERROR_OK;
}


/*
 * Check a directory-path arugment in a request line.
 * Book name must be either form:
 *
 *    "/" SUBBOOK_NAME *( "/" DIRECTORY_NAME )
 *    "/"
 *
 * `book' and `content_type' is the parse result of book name in the
 * request, set by parse_and_check_book_name().
 *
 * If the arguments are valid, the function returns EB_ERROR_OK,
 * otherwise it returns an error code.
 */
static EBNet_Error_Code
check_directory_path(book, content_type, path)
    Book *book;
    EBNet_Content_Type content_type;
    const char *path;
{
    char subbook_name[EB_MAX_FILE_NAME_LENGTH + 1];
    size_t subbook_name_length;
    EBNet_Error_Code error_code;
    char *slash;

    if (*path != '/')
	return EBNET_ERROR_BAD_PATH;

    if (*(path + 1) != '\0') {
	/*
	 * Get a subbook name and check whether `book' has a subbook
	 * with that name.
	 */
	slash = strchr(path + 1, '/');
	if (slash == NULL)
	    subbook_name_length = strlen(path + 1);
	else
	    subbook_name_length = slash - (path + 1);
	if (subbook_name_length == 0
	    || EB_MAX_DIRECTORY_NAME_LENGTH < subbook_name_length)
	    return EBNET_ERROR_BAD_SUBBOOK_NAME;
	memcpy(subbook_name, path + 1, subbook_name_length);
	*(subbook_name + subbook_name_length) = '\0';

	error_code = check_subbook_name(book, content_type, subbook_name);
	if (error_code != EBNET_ERROR_OK)
	    return error_code;

	/*
	 * Check rest of the path, if exists.
	 */
	if (slash != NULL) {
	    error_code = check_path_parent_directory(slash);
	    if (error_code != EBNET_ERROR_OK)
		return error_code;
	}
    }

    return EBNET_ERROR_OK;
}


/*
 * Perform subbook name check on `subbook_name'.
 *
 * `book' and `content_type' is the parse result of book name in the
 * request, set by parse_and_check_book_name().
 *
 * If the arguments are valid, the function returns EB_ERROR_OK,
 * otherwise it returns an error code.
 */
static EBNet_Error_Code
check_subbook_name(book, content_type, subbook_name)
    Book *book;
    EBNet_Content_Type content_type;
    const char *subbook_name;
{
    EB_Subbook_Code subbook;
    EBNet_Error_Code error_code;

    error_code = check_path_parent_directory(subbook_name);
    if (error_code != EBNET_ERROR_OK)
	return error_code;

    if (content_type == EBNET_CONTENT_TYPE_BOOK)
	subbook = find_subbook(book, subbook_name);
    else
	subbook = find_appendix_subbook(book, subbook_name);

    if (subbook == EB_SUBBOOK_INVALID)
	return EBNET_ERROR_BAD_SUBBOOK_NAME;

    return EBNET_ERROR_OK;
}


/*
 * Perform root file check on `file_name'.
 *
 * `file_name' must be "catalog", "catalogs" or "language".
 * The file name is case insensitive and it may have the suffix ".",
 * ".ebz" or ".org" and a version number ";DIGIT".
 *
 * If the file name is valid, the function returns EB_ERROR_OK,
 * otherwise it returns an error code.
 */
static EBNet_Error_Code
check_root_file_name(file_name)
    const char *file_name;
{
    size_t file_name_length;
    EBNet_Error_Code error_code;

    error_code = check_path_parent_directory(file_name);
    if (error_code != EBNET_ERROR_OK)
	return error_code;

    file_name_length = strlen(file_name);
    if (2 <= file_name_length
	&& *(file_name + file_name_length - 2) == ';'
	&& isdigit(*(file_name + file_name_length - 1))) {
	file_name_length -= 2;
    }
    if (1 <= file_name_length
	&& *(file_name + file_name_length - 1) == '.') {
	file_name_length -= 1;
    }

    if (strncasecmp(file_name, "catalog", file_name_length) != 0
	&& strncasecmp(file_name, "catalogs", file_name_length) != 0
	&& strncasecmp(file_name, "language", file_name_length) != 0)
	return EBNET_ERROR_BAD_PATH;

    return EBNET_ERROR_OK;
}


/*
 * Perform security check on `path'.
 *
 * If `path' contains "/..", the function returns EBNET_ERROR_BAD_PATH,
 * otherwise it returns EB_ERROR_OK.
 */
static EBNet_Error_Code
check_path_parent_directory(path)
    const char *path;
{
    const char *slash;
    const char *p = path;

    for (;;) {
	slash = strchr(p, '/');
	if (slash == NULL)
	    break;
	if (*(slash + 1) == '.' && *(slash + 2) == '.')
	    return EBNET_ERROR_BAD_PATH;
	p = slash + 1;
    }

    return EBNET_ERROR_OK;
}


/*
 * Truncate a string to a specified length.
 */
static void
truncate_string(string, length)
    char *string;
    size_t length;
{
    char *s;
    size_t i;

    for (i = 0, s = string; i < length && *s != '\0'; i++, s++)
	; /* nothing to do */

    *s = '\0';
}

    

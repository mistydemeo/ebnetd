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
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>

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

#include "defs.h"
#include "fakelog.h"
#include "ioall.h"
#include "linebuf.h"

#include "ndtp.h"

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
static int command_L EBNETD_P((char *, char *));
static int command_Q EBNETD_P((char *, char *));
static int command_A EBNETD_P((char *, char *));
static int command_u EBNETD_P((char *, char *));
static int command_P EBNETD_P((char *, char *));
static int command_F EBNETD_P((char *, char *));
static int command_S EBNETD_P((char *, char *));
static int command_I EBNETD_P((char *, char *));
static int command_v EBNETD_P((char *, char *));
static int command_T EBNETD_P((char *, char *));
static int command_t EBNETD_P((char *, char *));
static int command_X EBNETD_P((char *, char *));
static int command_XL EBNETD_P((char *, char *));
static int command_XI EBNETD_P((char *, char *));
static int command_XB EBNETD_P((char *, char *));
static int command_Xb EBNETD_P((char *, char *));
static int command_XS EBNETD_P((char *, char *));
static int book_number_to_name EBNETD_P((char *));
static void reverse_word EBNETD_P((char *));
static void iso8859_1_to_ascii_str EBNETD_P((char *));
static void iso8859_1_to_ascii_mem EBNETD_P((char *, size_t));

/*
 * NDTP command table.
 */
typedef struct {
    char name;
    int (*function) EBNETD_P((char *, char *));
} NDTP_Command;

static const NDTP_Command ndtp_commands[] = {
    {'L', command_L},
    {'Q', command_Q},
    {'A', command_A},
    {'u', command_u},
    {'P', command_P},
    {'F', command_F},
    {'S', command_S},
    {'I', command_I},
    {'v', command_v},
    {'T', command_T},
    {'t', command_t},
    {'X', command_X},
    {'\0', NULL}
};

/*
 * Line buffer to comunicate with a client.
 */
static Line_Buffer line_buffer;

/*
 * Hookset for processing text in CD-ROM books.
 */
static EB_Hookset ndtp_hookset;

/*
 * Current book.
 */
static Book *current_book;

/*
 * Current subbook name.
 */
static char current_subbook_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

/*
 * Current font height.
 */
static int current_font_height;

/*
 * Communicate with a client by NDTP.
 */
int
protocol_main()
{
    char line[NDTP_MAX_LINE_LENGTH + 1];
    const NDTP_Command *command;
    char command_name[2];
    ssize_t line_length;

    current_book = NULL;
    current_subbook_name[0] = '\0';
    current_font_height = 0;

    initialize_line_buffer(&line_buffer);
    bind_file_to_line_buffer(&line_buffer, accepted_in_file);
    set_line_buffer_timeout(&line_buffer, idle_timeout);
    eb_initialize_hookset(&ndtp_hookset);
    set_common_hooks(&ndtp_hookset);

    for (;;) {
	/*
	 * Read a line received from the client.
	 */
	line_length = read_line_buffer(&line_buffer, line,
	    NDTP_MAX_LINE_LENGTH);
	if (line_length < 0)
	    goto die;
	else if (line_length == 0)
	    continue;

	/*
	 * Check for the line length.
	 */
	if (NDTP_MAX_LINE_LENGTH <= line_length) {
	    if (skip_line_buffer(&line_buffer) < 0)
		goto die;
	    *(line + NDTP_SHORT_BUFFER_LENGTH) = '\0';
	    syslog(LOG_INFO, "too long request line: line=%s..., host=%s(%s)",
		line, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
		goto die;
	    continue;
	}
	syslog(LOG_DEBUG, "debug: received line: %s", line);

	/*
	 * Scan the command table.
	 */
	for (command = ndtp_commands; command->name != '\0'; command++) {
	    if (command->name == *line)
		break;
	}
	if (command->name == '\0') {
	    *(line + 1) = '\0';
	    syslog(LOG_INFO, "unknown command: command=%s, host=%s(%s)",
		line, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
		goto die;

	    continue;
	}

	/*
	 * Dispatch.
	 */
	command_name[0] = *line;
	command_name[1] = '\0';
	if ((command->function)(command_name, line + 1) < 0)
	    goto die;

	/*
	 * Quit when the received command is `Q'. (Quit even when
	 * ndtpd receives too many arguments for `Q'.)
	 */
	if (*line == 'Q')
	    break;
    }

    finalize_line_buffer(&line_buffer);
    eb_finalize_hookset(&ndtp_hookset);
    return 0;

  die:
    finalize_line_buffer(&line_buffer);
    eb_finalize_hookset(&ndtp_hookset);
    return -1;
}


/*
 * Command `L'.  Change the current book/subbook.
 * It takes an argument which represents a book/subbook name.
 */
static int
command_L(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    char book_name[MAX_BOOK_NAME_LENGTH + 1];
    char subbook_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    EB_Subbook_Code subbook;
    EB_Subbook_Code appendix_subbook;
    char *slash;

    /*
     * Unset the current book.
     */
    if (current_book != NULL) {
	eb_unset_subbook(&current_book->book);
	eb_unset_appendix_subbook(&current_book->appendix);
	current_book = NULL;
    }
    current_subbook_name[0] = '\0';
    current_font_height = 0;

    /*
     * If `argument' is `nodict', unset the current book/subbook.
     */
    if (strcmp(argument, "nodict") == 0) {
	if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	    return -1;
	syslog(LOG_INFO, "succeeded: command=%s, book=nodict, host=%s(%s)",
	    command, client_host_name, client_address);
	goto failed;
    }

    /*
     * If the argument doesn't contain a slash (`/'), it must be a book
     * number.  We convert it to the corresponding book name.
     */
    slash = strchr(argument, '/');
    if (slash == NULL) {
	if (book_number_to_name(argument) < 0) {
	    syslog(LOG_INFO, "unknown book: command=%s, book=%s, host=%s(%s)",
		command, argument, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
		return -1;
	    goto failed;
	}

	/*
	 * The number `0' is converted to `nodict'.
	 * Unset the current book/subbook.
	 */
	if (strcmp(argument, "nodict") == 0) {
	    if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
		return -1;
	    syslog(LOG_INFO, "succeeded: command=%s, book=nodict, host=%s(%s)",
		command, client_host_name, client_address);
	    return 0;
	}

	/*
	 * Find a slash (`/') again.
	 */
	slash = strchr(argument, '/');
    }

    /*
     * Get book and subbook names separated by a slash (`/').
     */
    strncpy(book_name, argument, (int)(slash - argument));
    *(book_name + (int)(slash - argument)) = '\0';
    strcpy(subbook_name, slash + 1);

    /*
     * Set the current book.
     */
    current_book = find_book(book_name);
    if (current_book == NULL) {
	syslog(LOG_INFO, "unknown book: command=%s, book=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Check for access permission to the book.
     */
    if (!current_book->permission_flag) {
	syslog(LOG_ERR, "denied: command=%s, book=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Get a ticket to access the book.
     * (Don't release the ticket when the current book is unset.)
     */
    if (current_book->max_clients != 0
	&& get_ticket(&current_book->ticket_stock) < 0) {
	syslog(LOG_INFO, "full of clients: command=%s, book=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Find the subbook in the current book.
     */
    subbook = find_subbook(current_book, subbook_name);
    if (subbook < 0) {
	syslog(LOG_INFO, "unknown book: command=%s, book=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Set the current subbook.
     */
    error_code = eb_set_subbook(&current_book->book, subbook);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s, host=%s(%s)",
	    eb_error_message(error_code), command, argument,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Find the subbook in the current appendix.
     */
    error_code = eb_is_appendix_bound(&current_book->appendix);
    if (error_code != EB_SUCCESS) {
	appendix_subbook = find_appendix_subbook(current_book, subbook_name);
	if (appendix_subbook < 0) {
	    syslog(LOG_INFO, "unknown appendix: command=%s, book=%s, \
host=%s(%s)",
		command, argument, client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
		return -1;
	    goto failed;
	}

	/*
	 * Set the current subbook.
	 */
	error_code = eb_set_appendix_subbook(&current_book->appendix,
	    appendix_subbook);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s, host=%s(%s)",
		eb_error_message(error_code), command, argument,
		client_host_name, client_address);
	    if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
		return -1;
	    goto failed;
	}
    }

    /*
     * Get the current subbook name.
     */
    error_code = eb_subbook_directory(&current_book->book,
	current_subbook_name);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s, host=%s(%s)",
	    eb_error_message(error_code), command, argument,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Set text hooks.
     */
    set_text_hooks(&ndtp_hookset);

    /*
     * Send a response.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s, host=%s(%s)",
	command, argument, client_host_name, client_address);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (current_book != NULL) {
	eb_unset_subbook(&current_book->book);
	eb_unset_appendix_subbook(&current_book->appendix);
	current_book = NULL;
    }
    return 0;
}


/*
 * Command `Q'.  Quit.
 */
static int
command_Q(command, argument)
    char *command;
    char *argument;
{
    /*
     * The argument should be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	return 0;
    }

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/*
 * Command `A'.  Get access permission.
 * The command is not supported.  It always sends the `succeeded' response.
 */
static int
command_A(command, argument)
    char *command;
    char *argument;
{
    /*
     * Send a success code.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$A\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/*
 * Command `u'.  List current users.
 * The command is not supported.  It always sends an empty list.
 */
static int
command_u(command, argument)
    char *command;
    char *argument;
{
    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send an empty list.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$U\n$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/* Index size of a hit list. */
#define HIT_LIST_LENGTH	50

/*
 * Command `P'.  Search a word.
 * It takes two arguments without a separater.
 * The first argument is a character which represents an index type.
 * The second argument represents a word to search.
 */
static int
command_P(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Character_Code character_code;
    EB_Hit hit_list[HIT_LIST_LENGTH];
    EB_Hit *hit;
    int hit_count;
    char position_buffer[MAX_STRING_LENGTH + 1];
    char heading_buffer1[MAX_STRING_LENGTH + 1];
    char heading_buffer2[MAX_STRING_LENGTH + 1];
    char *heading;
    char *previous_heading;
    ssize_t heading_length;
    EB_Position previous_text_position;
    char word[EB_MAX_WORD_LENGTH + 1];
    size_t word_length;
    char method;
    int n;
    int i;

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Set a word to search and a search method.
     */
    word_length = strlen(argument) - 1;
    if (EB_MAX_WORD_LENGTH < word_length) {
	syslog(LOG_INFO, "too long word: command=%s, host=%s(%s)", 
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }
    if (*argument == '\0') {
	syslog(LOG_INFO, "empty method name: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }
    strcpy(word, argument + 1);
    method = *argument;

    /*
     * Check for the search method.
     */
    if (strchr("akAKj", method) == NULL) {
	syslog(LOG_INFO, "bad search method: command=%s, method=%c, \
host=%s(%s)",
	    command, method, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * For syslog output, replace TAB in `argument' with semicolon.
     */
    if (method == 'j') {
	char *p = argument + 1;
	for (;;) {
	    p = strchr(p, '\t');
	    if (p == NULL)
		break;
	    *p++ = ';';
	}
    }

    /*
     * Submit a search request.
     *
     * Before submission, remove `*' in the tail of the word and choose
     * a search function.  If the word is written in ISO 8859-1 and it
     * ends with `\xa1\xf6' (upside-down `!' and umlaut accent `o'), it
     * is recognized as asterisk (`*') of JIS X 0208 by mistake.
     */
    if (method == 'j') {
	char *keywords[EB_MAX_KEYWORDS + 2];
	char *p;
	char *tab;

	/*
	 * Split keywords with a TAB.  We split (EB_MAX_KEYWORDS + 1)
	 * keywords maximum because "too many keywords" error can
	 * be detected by EB Library.
	 */
	p = word;
	i = 0; 
	while (i < EB_MAX_KEYWORDS + 1) {
	    keywords[i++] = p;
	    tab = strchr(p, '\t');
	    if (tab == NULL)
		break;
	    *tab = '\0';
	    p = tab + 1;
	}
	keywords[i] = NULL;
	error_code = eb_search_keyword(&current_book->book, 
	    (const char * const *) keywords);

    } else if (1 <= word_length && *(word + word_length - 1) == '*') {
	/*
	 * The word is terminated by "*".
	 */
	*(word + word_length - 1) = '\0';
	if (method == 'a' || method == 'k') {
	    error_code = eb_search_word(&current_book->book, word);
	} else {
	    reverse_word(word);
	    error_code = eb_search_endword(&current_book->book, word);
	}
    } else if (2 <= word_length
	/*
	 * The word is terminated by an asterisk of JIS X 0208.
	 */
	&& *((unsigned char *)(word + word_length - 2)) == 0xa1
	&& *((unsigned char *)(word + word_length - 1)) == 0xf6) {
	*(word + word_length - 2) = '\0';
	if (method == 'a' || method == 'k') {
	    error_code = eb_search_word(&current_book->book, word);
	} else {
	    reverse_word(word);
	    error_code = eb_search_endword(&current_book->book, word);
	}
    } else {
	/*
	 * Exact word search.
	 */
	error_code = eb_search_exactword(&current_book->book, word);
    }

    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, method=%c, word=%s, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, method, argument + 1, client_host_name,
	    client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$0\n$$\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Get hit entries and send them to a client.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$0\n") <= 0)
	return -1;

    hit_count = 0;
    heading = heading_buffer1;
    previous_heading = heading_buffer2;
    *previous_heading = '\0';
    previous_text_position.page = 0;
    previous_text_position.offset = 0;
    n = 0;
    error_code = eb_character_code(&current_book->book, &character_code);
    if (error_code != EB_SUCCESS)
	character_code = EB_CHARCODE_JISX0208;

    while (hit_count < max_hits || max_hits == 0) {
	/*
	 * Get hit entries.
	 */
	error_code = eb_hit_list(&current_book->book, HIT_LIST_LENGTH,
	    hit_list, &n);
	if (error_code != EB_SUCCESS || n == 0)
	    break;

	for (i = 0, hit = hit_list; i < n; i++, hit_count++, hit++) {
	    /*
	     * Seek the current subbook.
	     */
	    error_code = eb_seek_text(&current_book->book, &hit->heading);
	    if (error_code != EB_SUCCESS) {
		syslog(LOG_INFO, "%s: command=%s, book=%s/%s, method=%c, \
word=%s, host=%s(%s)",
		    eb_error_message(error_code), command, current_book->name,
		    current_subbook_name, method, argument + 1,
		    client_host_name, client_address);
		continue;
	    }

	    /*
	     * Get heading.
	     */
	    error_code = eb_read_heading(&current_book->book,
		&current_book->appendix, &ndtp_hookset, NULL, 
		MAX_STRING_LENGTH, heading, &heading_length);
	    if (error_code != EB_SUCCESS) {
		syslog(LOG_INFO, "%s: command=%s, book=%s/%s, method=%c, \
word=%s, host=%s(%s)",
		    eb_error_message(error_code), command, current_book->name,
		    current_subbook_name, method, argument + 1,
		    client_host_name, client_address);
		break;
	    }
	    if (error_code != EB_SUCCESS || heading_length == 0)
		continue;

	    *(heading + heading_length) = '\0';
	    if (character_code == EB_CHARCODE_ISO8859_1)
		iso8859_1_to_ascii_mem(heading, heading_length);

	    /*
	     * Ignore a hit entry if its heading and text location are
	     * equal to those of the previous hit entry.
	     */
	    if (previous_text_position.page == hit->text.page
		&& previous_text_position.offset == hit->text.offset 
		&& strcmp(heading, previous_heading) == 0)
		continue;

	    /*
	     * Output the heading and text position.
	     */
	    if (write_string_all(accepted_out_file, idle_timeout, heading)
		<= 0)
		return -1;
	    sprintf(position_buffer, "\n%x:%x\n", hit->text.page,
		hit->text.offset);
	    if (write_string_all(accepted_out_file, idle_timeout,
		position_buffer) <= 0)
		return -1;

	    /*
	     * Keep the last message, page, and offset, to remove
	     * duplicated entries.
	     */
	    if (heading == heading_buffer1) {
		heading = heading_buffer2;
		previous_heading = heading_buffer1;
	    } else {
		heading = heading_buffer1;
		previous_heading = heading_buffer2;
	    }
	    previous_text_position.page = hit->text.page;
	    previous_text_position.offset = hit->text.offset;
	}
    }

    if (n < 0) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, method=%c, word=%s, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, method, argument + 1, client_host_name,
	    client_address);
    }
    if (write_string_all(accepted_out_file, idle_timeout, "$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, method=%c, \
word=%s, matches=%d, host=%s(%s)",
	command, current_book->name, current_subbook_name, method,
	argument + 1, hit_count, client_host_name, client_address);
    return 0;
}


/*
 * Command `F'.  Send a page.
 * It takes an argument which represents a page to send.
 * The page number must be a hexadecimal integer.
 */
static int
command_F(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Position position;
    char message[EB_SIZE_PAGE + 3];  /* "$F" + page + "\n" */
    char *end_p;
    ssize_t text_length;

    /*
     * Set header and footer.
     */
    message[0] = '$';
    message[1] = 'F';
    message[EB_SIZE_PAGE + 2] = '\n';

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse an page number.
     */
    position.offset = 0;
    position.page = strtol(argument, &end_p, 16);
    if (!isxdigit(*argument) || *end_p != '\0') {
	syslog(LOG_INFO, "bad page number: command=%s, page=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Read the page.
     */
    error_code = eb_seek_text(&current_book->book, &position);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, page=%d, host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, position.page, client_host_name,
	    client_address);
	memset(message + 2, '\0', EB_SIZE_PAGE);
	if (write_all(accepted_out_file, idle_timeout, message,
	    EB_SIZE_PAGE + 3) <= 0)
	    return -1;
	return 0;
    }
    error_code = eb_read_rawtext(&current_book->book, EB_SIZE_PAGE,
	message + 2, &text_length);
    if (error_code != EB_SUCCESS || text_length != EB_SIZE_PAGE) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, page=%d, host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, position.page, client_host_name,
	    client_address);
	memset(message + 2, '\0', EB_SIZE_PAGE);
	if (write_all(accepted_out_file, idle_timeout, message,
	    EB_SIZE_PAGE + 3) <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send the page to the client ("$F" + page + "\n").
     */
    if (write_all(accepted_out_file, idle_timeout, message,
	EB_SIZE_PAGE + 3) <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, page=%d, host=%s(%s)",
	command, current_book->name, current_subbook_name, position.page,
	client_host_name, client_address);
    return 0;
}


/*
 * Command `S'.  Send text.
 * It takes two arguments; page number and offset separeted by a colon (`:').
 * Both the page number and offset must be hexadecimal integers.
 */
static int
command_S(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Position position;
    EB_Character_Code character_code;
    char text[EB_SIZE_PAGE];
    char *end_p;
    char *offset_p;
    size_t rest_text_length;
    size_t text_length_argument;
    ssize_t actual_text_length;

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse a page number.
     */
    position.page = strtol(argument, &end_p, 16);
    if (!isxdigit(*argument) || *end_p != ':') {
	syslog(LOG_INFO, "bad position: command=%s, position=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse an offset number.
     */
    offset_p = end_p + 1;
    position.offset = strtol(offset_p, &end_p, 16);
    if (!isxdigit(*offset_p) || *end_p != '\0') {
	syslog(LOG_INFO, "bad position: command=%s, position=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Get character code of the current book.
     */
    error_code = eb_character_code(&current_book->book, &character_code);
    if (error_code != EB_SUCCESS)
	character_code = EB_CHARCODE_JISX0208;

    /*
     * Seek the current subbook.
     */
    error_code = eb_seek_text(&current_book->book, &position);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, position=%s, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$1\n$$\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Read text and send them to the client.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$1\n") <= 0)
	return -1;

    rest_text_length = max_text_size;
    for (;;) {
	if (EB_SIZE_PAGE - 1 < rest_text_length || max_text_size == 0)
	    text_length_argument = EB_SIZE_PAGE - 1;
	else
	    text_length_argument = rest_text_length;
	if (text_length_argument <= 0)
	    break;

	error_code = eb_read_text(&current_book->book, &current_book->appendix,
	    &ndtp_hookset, NULL, text_length_argument, text,
	    &actual_text_length);
	if (error_code != EB_SUCCESS || actual_text_length == 0)
	    break;
	rest_text_length -= actual_text_length;
	if (character_code == EB_CHARCODE_ISO8859_1)
	    iso8859_1_to_ascii_mem(text, actual_text_length);
	if (write_all(accepted_out_file, idle_timeout, text,
	    actual_text_length) <= 0)
	    return -1;
    }

    if (write_string_all(accepted_out_file, idle_timeout, "\n$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, position=%s, \
host=%s(%s)",
	command, current_book->name, current_subbook_name, argument,
        client_host_name, client_address);
    return 0;
}


/*
 * Command `I'.  Send an index list.
 * The page numbers in the table are dummy except for menu search.
 */
static int
command_I(command, argument)
    char *command;
    char *argument;
{
    char message[MAX_STRING_LENGTH + 1];
    EB_Position position;

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send response data.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$I\n") <= 0)
	return -1;
    if (eb_have_word_search(&current_book->book)) {
	if (write_string_all(accepted_out_file, idle_timeout, "IA 1\nIK 1\n")
	    <= 0)
	    return -1;
    }
    if (eb_have_endword_search(&current_book->book)) {
	if (write_string_all(accepted_out_file, idle_timeout, "BA 1\nBK 1\n")
	    <= 0)
	    return -1;
    }
    if (eb_menu(&current_book->book, &position) == EB_SUCCESS) {
	sprintf(message, "HA %x\n", position.page);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	    return -1;
    }
    if (eb_copyright(&current_book->book, &position) == EB_SUCCESS) {
	sprintf(message, "OK %x\n", position.page);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	    return -1;
    }
    if (eb_have_keyword_search(&current_book->book)) {
	if (write_string_all(accepted_out_file, idle_timeout, "JO 1\n") <= 0)
	    return -1;
    }

    if (write_string_all(accepted_out_file, idle_timeout, "$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, host=%s(%s)",
	command, current_book->name, current_subbook_name, client_host_name,
	client_address);
    return 0;
}


/*
 * Command `v'.  Output version number of the server.
 */
static int
command_v(command, argument)
    char *command;
    char *argument;
{
    char message[MAX_STRING_LENGTH + 1];

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    sprintf(message, "$v%s version %s\n", program_name, program_version);
    if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/*
 * Command `T'.  Output a book list.
 */
static int
command_T(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    EB_Character_Code character_code;
    Book *book;
    char message[MAX_STRING_LENGTH + 1];
    char title[EB_MAX_TITLE_LENGTH + 1];
    int subbook_count;
    int i, j;

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send a book list.
     */
    for (book = book_registry, i = 1;
	 book != NULL; book = book->next) {
	if (!eb_is_bound(&book->book))
	    continue;
	error_code = eb_subbook_list(&book->book, subbook_list,
	    &subbook_count);
	if (error_code != EB_SUCCESS || !book->permission_flag)
	    continue;
	error_code = eb_character_code(&book->book, &character_code);
	if (error_code != EB_SUCCESS)
	    character_code = EB_CHARCODE_JISX0208;
	for (j = 0; j < subbook_count; j++, i++) {
	    error_code = eb_subbook_title2(&book->book, subbook_list[j],
		title);
	    if (error_code != EB_SUCCESS)
		strcpy(title, "(not available)");
	    sprintf(message, "%d\t%s\n", i, title);
	    if (character_code == EB_CHARCODE_ISO8859_1)
		iso8859_1_to_ascii_str(message);
	    if (write_string_all(accepted_out_file, idle_timeout, message)
		<= 0)
		return -1;
	}
    }
    if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/*
 * Command `t'.  List current users and books.
 * The command is not supported.  It always sends an empty list.
 */
static int
command_t(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    EB_Character_Code character_code;
    Book *book;
    char message[MAX_STRING_LENGTH + 1];
    char title[EB_MAX_TITLE_LENGTH + 1];
    char directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_count;
    int i, j;

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send a book list.
     */
    for (book = book_registry, i = 1;
	 book != NULL; book = book->next) {
	if (!eb_is_bound(&book->book))
	    continue;
	error_code = eb_subbook_list(&book->book, subbook_list,
	    &subbook_count);
	if (error_code != EB_SUCCESS || !book->permission_flag)
	    continue;
	error_code = eb_character_code(&book->book, &character_code);
	if (error_code != EB_SUCCESS)
	    character_code = EB_CHARCODE_JISX0208;

	for (j = 0; j < subbook_count; j++, i++) {
	    error_code = eb_subbook_directory2(&book->book, subbook_list[j],
		directory);
	    if (error_code != EB_SUCCESS)
		strcpy(directory, "*");
	    error_code = eb_subbook_title2(&book->book, subbook_list[j],
		title);
	    if (error_code != EB_SUCCESS)
		strcpy(title, "(not available)");
	    sprintf(message, "%d\t%s\t%s/%s\t%d\t%d\t%d\n", i, title,
		book->name, directory, 0, book->max_clients, idle_timeout);
	    if (character_code == EB_CHARCODE_ISO8859_1)
		iso8859_1_to_ascii_str(message);
	    if (write_string_all(accepted_out_file, idle_timeout, message)
		<= 0)
		return -1;
	}
    }
    if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    return 0;
}


/*
 * `X' prefixed Commands.
 */
static int
command_X(command, argument)
    char *command;
    char *argument;
{
    switch (*argument) {
    case 'I':
	return command_XI("XI", argument + 1);
    case 'L':
	return command_XL("XL", argument + 1);
    case 'B':
	return command_XB("XB", argument + 1);
    case 'b':
	return command_Xb("Xb", argument + 1);
    case 'S':
	return command_XS("XS", argument + 1);
    }

    syslog(LOG_INFO, "unknown command: command=%s, host=%s(%s)",
	command, client_host_name, client_address);
    if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	return -1;

    return 0;
}


/*
 * Command `XI'.  Send a bitmap size list.
 */
static int
command_XI(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Font_Code font_list[EB_MAX_FONTS];
    int height;
    int font_count;
    char message[MAX_STRING_LENGTH + 1];
    int i;

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Send response data.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$I\n") <= 0)
	return -1;

    error_code = eb_font_list(&current_book->book, font_list, &font_count);
    if (error_code != EB_SUCCESS)
	font_count = 0;
    for (i = 0; i < font_count; i++) {
	if (eb_font_height2(font_list[i], &height) != EB_SUCCESS)
	    continue;
	sprintf(message, "%d\n", height);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	    return -1;
    }
    if (write_string_all(accepted_out_file, idle_timeout, "$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, host=%s(%s)",
	command, current_book->name, current_subbook_name,
	client_host_name, client_address);
    return 0;
}


/*
 * Command `XL'.  Set the current bitmap size.
 */
static int
command_XL(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Font_Code font_code;
    int height;
    char *end_p;

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Unset the current bitmap size.
     */
    eb_unset_font(&current_book->book);
    current_font_height = 0;

    /*
     * Parse an argument.
     */
    height = strtol(argument, &end_p, 10);
    if (!isdigit(argument[0]) || *end_p != '\0') {
	syslog(LOG_INFO, "bad bitmap height: command=%s, bitmap-size=%s, \
host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * If height is `0', unset the bitmap size.
     */
    if (height == 0) {
	if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	    return -1;
	syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, bitmap-size=0, \
host=%s(%s)",
	    command, current_book->name, current_subbook_name,
	    client_host_name, client_address);
	set_text_hooks(&ndtp_hookset);
	return 0;
    }

    /*
     * Check for the font height.
     */
    switch (height) {
    case 16:
	font_code = EB_FONT_16;
	break;
    case 24:
	font_code = EB_FONT_24;
	break;
    case 30:
	font_code = EB_FONT_30;
	break;
    case 48:
	font_code = EB_FONT_48;
	break;
    default:
	syslog(LOG_INFO, "bad bitmap height: command=%s, book=%s/%s, \
bitmap-size=%d, host=%s(%s)",
	    command, current_book->name, current_subbook_name, height,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }

    /*
     * Set the current bitmap size.
     */
    error_code = eb_set_font(&current_book->book, font_code);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, height, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	goto failed;
    }
    current_font_height = height;
    set_bitmap_hooks(&ndtp_hookset);

    /*
     * Send a response.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$*\n") <= 0)
	return -1;
    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
	command, current_book->name, current_subbook_name, height,
	client_host_name, client_address);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (current_book != NULL) {
	eb_unset_font(&current_book->book);
	current_font_height = 0;
	set_text_hooks(&ndtp_hookset);
    }
    return 0;
}


/*
 * Command `XB'.  Send bitmap data of all local characters defined in
 * the current subbook.
 */
static int
command_XB(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    char bitmap[EB_SIZE_WIDE_FONT_48];
    char xbm[EB_SIZE_NARROW_FONT_48_XBM];
    char header[MAX_STRING_LENGTH + 1];
    int width;
    int start_character_number;
    int end_character_number;
    int ch;
    size_t xbm_length;

    /*
     * The argument must be empty.
     */
    if (*argument != '\0') {
	syslog(LOG_INFO, "too many arguments: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Check for the current book and bitmap size.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }
    if (current_font_height == 0) {
	syslog(LOG_INFO, "no current bitmap size: command=%s, book=%s/%s, \
host=%s(%s)",
	    command, current_book->name, current_subbook_name,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    if (write_string_all(accepted_out_file, idle_timeout, "$I\n") <= 0)
	return -1;

    /*
     * Send bitmap data of narrow characters.
     */
    do {
	if (!eb_have_narrow_font(&current_book->book))
	    break;
	error_code = eb_narrow_font_width(&current_book->book, &width);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	    break;
	}

	error_code = eb_narrow_font_start(&current_book->book,
	    &start_character_number);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	}

	error_code = eb_narrow_font_end(&current_book->book,
	     &end_character_number);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	    break;
	}

	ch = start_character_number;
	while (0 <= ch) {
	    error_code = eb_narrow_font_character_bitmap(&current_book->book,
		ch, bitmap);
	    if (error_code != EB_SUCCESS)
		continue;
	    eb_bitmap_to_xbm(bitmap, width, current_font_height, xbm,
		&xbm_length);
	    sprintf(header, "$=h%04x\n", ch);
	    if (write_string_all(accepted_out_file, idle_timeout, header) <= 0)
		return -1;
	    if (write_all(accepted_out_file, idle_timeout, xbm, xbm_length)
		<= 0)
		return -1;
	    error_code = eb_forward_narrow_font_character(&current_book->book,
		1, &ch);
	    if (error_code != EB_SUCCESS)
		break;
	}
    } while (0);

    /*
     * Send bitmap data of wide characters.
     */
    do {
	if (!eb_have_wide_font(&current_book->book))
	    break;
	error_code = eb_wide_font_width(&current_book->book, &width);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	    break;
	}

	error_code = eb_wide_font_start(&current_book->book,
	    &start_character_number);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	    break;
	}

	error_code= eb_wide_font_end(&current_book->book,
	    &end_character_number);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
		eb_error_message(error_code), command, current_book->name,
		current_subbook_name, current_font_height, client_host_name,
		client_address);
	    break;
	}

	ch = start_character_number;
	while (0 <= ch) {
	    error_code = eb_wide_font_character_bitmap(&current_book->book, ch,
		bitmap);
	    if (error_code != EB_SUCCESS)
		continue;
	    eb_bitmap_to_xbm(bitmap, width, current_font_height, xbm,
		&xbm_length);
	    sprintf(header, "$=z%04x\n", ch);
	    if (write_string_all(accepted_out_file, idle_timeout, header) <= 0)
		return -1;
	    if (write_all(accepted_out_file, idle_timeout, xbm, xbm_length)
		<= 0)
		return -1;
	    error_code = eb_forward_wide_font_character(&current_book->book,
		1, &ch);
	    if (error_code != EB_SUCCESS)
		break;
	}
    } while (0);

    /*
     * End of the response data.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, bitmap-size=%d, \
host=%s(%s)",
	command, current_book->name, current_subbook_name,
	current_font_height, client_host_name, client_address);
    return 0;
}


/*
 * Command `Xb'.  Send bitmap data of a local character.
 */
static int
command_Xb(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    char bitmap[EB_SIZE_WIDE_FONT_48];
    char xbm[EB_SIZE_WIDE_FONT_48_XBM];
    char *end_p;
    int character_number;
    int character_type;
    int width;
    size_t xbm_length;

    /*
     * Check for the current book and bitmap size.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }
    if (current_font_height == 0) {
	syslog(LOG_INFO, "no current bitmap size: command=%s, book=%s/%s, \
host=%s(%s)",
	    command, current_book->name, current_subbook_name, 
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse a charcter number;
     */
    character_type = *argument;
    character_number = strtol(argument + 1, &end_p, 16);
    if (!isxdigit(*(argument + 1))
	|| *end_p != '\0'
	|| (character_type != 'h' && character_type != 'z')) {
	syslog(LOG_INFO, "bad bitmap height: command=%s, character=%s, \
host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Get width of the character.
     */
    if (character_type == 'h')
	error_code = eb_narrow_font_width(&current_book->book, &width);
    else
	error_code = eb_wide_font_width(&current_book->book, &width);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
character=%s, host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, current_font_height, argument,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Get bitmap data of the character.
     */
    if (character_type == 'h') {
	error_code = eb_narrow_font_character_bitmap(&current_book->book,
	    character_number, bitmap);
    } else {
	error_code = eb_wide_font_character_bitmap(&current_book->book,
	    character_number, bitmap);
    }
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, bitmap-size=%d, \
character=%s, host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, current_font_height, argument,
	    client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Get XBM data of the character.
     */
    eb_bitmap_to_xbm(bitmap, width, current_font_height, xbm, &xbm_length);

    /*
     * Send bitmap data of the character.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "$I\n") <= 0)
	return -1;
    if (write_all(accepted_out_file, idle_timeout, xbm, xbm_length) <= 0)
	return -1;
    if (write_string_all(accepted_out_file, idle_timeout, "$$\n") <= 0)
	return -1;

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, bitmap-size=%d, \
character=%s, host=%s(%s)",
	command, current_book->name, current_subbook_name, 
	current_font_height, argument, client_host_name, client_address);

    return 0;
}


/*
 * Command `XS'.  Tell location of the previous or next paragraph.
 */
static int
command_XS(command, argument)
    char *command;
    char *argument;
{
    EB_Error_Code error_code;
    EB_Position position;
    char message[MAX_STRING_LENGTH + 1];
    char *offset_p;
    char *end_p;
    int sign;

    /*
     * Check for the current book.
     */
    if (current_book == NULL) {
	syslog(LOG_INFO, "no current book: command=%s, host=%s(%s)",
	    command, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$<\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse an optional sign.
     */
    if (*argument != '+' && *argument != '-') {
	syslog(LOG_INFO, "bad direction sign: command=%s, position=%s, \
host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }
    sign = *argument;

    /*
     * Parse a page number.
     */
    position.page = strtol(argument + 1, &end_p, 16);
    if (!isxdigit(*(argument + 1)) || *end_p != ':') {
	syslog(LOG_INFO, "bad position: command=%s, position=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Parse an offset number.
     */
    offset_p = end_p + 1;
    position.offset = strtol(offset_p, &end_p, 16);
    if (!isxdigit(*offset_p) || *end_p != '\0') {
	syslog(LOG_INFO, "bad position: command=%s, position=%s, host=%s(%s)",
	    command, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$?\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Seek the current subbook.
     */
    error_code = eb_seek_text(&current_book->book, &position);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, position=%s, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, argument, client_host_name, client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$1\n$$\n") <= 0)
	    return -1;
	return 0;
    }

    /*
     * Forward or backward the text.
     */
    if (sign == '+') {
	error_code = eb_forward_text(&current_book->book,
	    &current_book->appendix);
    } else if (sign == '-') {
	error_code = eb_backward_text(&current_book->book,
	    &current_book->appendix);
    }
    if (error_code == EB_SUCCESS)
	error_code = eb_tell_text(&current_book->book, &position);

    /*
     * Output response.
     */
    if (error_code == EB_SUCCESS) {
	sprintf(message, "$XS%x:%x\n", position.page, position.offset);
	if (write_string_all(accepted_out_file, idle_timeout, message) <= 0)
	    return -1;
    } else if (error_code == EB_ERR_END_OF_CONTENT) {
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
    } else {
	syslog(LOG_INFO, "%s: command=%s, book=%s/%s, position=%s, \
host=%s(%s)",
	    eb_error_message(error_code), command, current_book->name,
	    current_subbook_name, argument, client_host_name,
	    client_address);
	if (write_string_all(accepted_out_file, idle_timeout, "$&\n") <= 0)
	    return -1;
	return 0;
    }

    syslog(LOG_INFO, "succeeded: command=%s, book=%s/%s, position=%s, \
host=%s(%s)",
	command, current_book->name, current_subbook_name, argument,
        client_host_name, client_address);
    return 0;
}


/*
 * Convert a book number `argument' to the corresponding book name.
 *
 * If succeeds, 0 is returned, and `argument' is overwritten.  Upon return,
 * `argument' represents the book name.  Otherwise, -1 is returned and 
 * `argument' is unchanged.
 */
static int
book_number_to_name(argument)
    char *argument;
{
    EB_Error_Code error_code;
    int number;
    Book *book;
    int subbook_count;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    char directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char *end_p;
    int i;

    /*
     * Get a book number.
     */
    number = strtol(argument, &end_p, 10);
    if (!isdigit(argument[0]) || *end_p != '\0')
	return -1;

    /*
     * If `argument' is `0', it is equivalent to `nodict'.
     * Unset the current book/subbook.
     */
    if (number == 0) {
	strcpy(argument, "nodict");
	return 0;
    }

    /*
     * Find `book/subbook' corresponding with `number'.
     */
    for (book = book_registry, i = 1;
	 book != NULL && i <= number; book = book->next) {
	if (!eb_is_bound(&book->book))
	    continue;
	error_code = eb_subbook_list(&book->book, subbook_list,
	    &subbook_count);
	if (error_code != EB_SUCCESS || !book->permission_flag)
	    continue;
	if (number < i + subbook_count) {
	    error_code = eb_subbook_directory2(&book->book,
		subbook_list[number - i], directory);
	    if (error_code != EB_SUCCESS)
		return -1;
	    sprintf(argument, "%s/%s", book->name, directory);
	    return 0;
	}
	i += subbook_count;
    }
    
    return -1;
}


/*
 * Reverse a word for ENDWORD SEARCH.
 * 
 * `word' is a word to reverse.  It must be an alphabetic word.
 * The reversed word is also put into `word'.
 */
static void
reverse_word(word)
    char *word;
{
    char buffer[NDTP_MAX_LINE_LENGTH + 1];
    unsigned char *p1, *p2;

    strcpy(buffer, word);
    p1 = (unsigned char *)word;
    p2 = (unsigned char *)buffer + strlen(word);

    *p2-- = '\0';
    while (*p1 != '\0') {
	if (*p1 == 0x8f) {
	    /*
	     * `*p1' is SS3.
	     * (It may be rejected by `eb_search_word', though.)
	     */
	    if (*(p1 + 1) == '\0' || *(p1 + 2) == '\0')
		break;
	    *p2 = *(p1 + 2);
	    *(p2 - 1) = *(p1 + 1);
	    *(p2 - 2) = *p1;
	    p1 += 3;
	    p2 -= 3;
	} else if ((0xa1 <= *p1 && *p1 <= 0xfe) || *p1 == 0x8e) {
	    /*
	     * `*p1' is SS2 or a character in JIS X 0208.
	     */
	    if (*(p1 + 1) == '\0')
		break;
	    *p2 = *(p1 + 1);
	    *(p2 - 1) = *p1;
	    p1 += 2;
	    p2 -= 2;
	} else {
	    /*
	     * Otherwise.
	     */
	    *p2 = *p1;
	    p1 += 1;
	    p2 -= 1;
	}
    }

    strcpy(word, (char *)p2 + 1);
}


/*
 * ISO 8859 1 to EUC-JP conversion table.
 */
static const char iso8859_1_to_euc_table[] = {
    ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',  /* 80 - 87 */
    ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',  /* 88 - 8f */
    ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',  /* 90 - 97 */
    ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',  /* 98 - 9f */
    ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   '|',   ' ',  /* a0 - a7 */
    '\'',  'C',   ' ',   '<',   ' ',   '-',   'R',   '~',  /* a8 - af */
    ' ',   ' ',   '2',   '3',   '\'',  'u',   '*',   '*',  /* b0 - b7 */
    ',' ,  '1',   ' ',   '>',   ' ',   ' ',   ' ',   ' ',  /* b8 - bf */
    'A',   'A',   'A',   'A',   'A',   'A',   ' ',   'C',  /* c0 - c7 */
    'E',   'E',   'E',   'E',   'I',   'I',   'I',   'I',  /* c8 - cf */
    'D',   'N',   'O',   'O',   'O',   'O',   'O',   'x',  /* d0 - d7 */
    'O',   'U',   'U',   'U',   'U',   'Y',   'P',   'B',  /* d8 - df */
    'a',   'a',   'a',   'a',   'a',   'a',   ' ',   'c',  /* e0 - e7 */
    'e',   'e',   'e',   'e',   'i',   'i',   'i',   'i',  /* e8 - ef */
    'o',   'n',   'o',   'o',   'o',   'o',   'o',   '/',  /* f0 - f7 */
    'o',   'u',   'u',   'u',   'u',   'y',   'p',   'y',  /* f8 - ff */
};


/*
 * Convert an ISO 8859-1 string to ASCII.
 */
static void
iso8859_1_to_ascii_str(string)
    char *string;
{
    unsigned char *strp = (unsigned char *)string;

    while (*strp != '\0') {
	if (0x80 <= *strp)
	    *(char *)strp = iso8859_1_to_euc_table[*strp - 0x80];
	strp++;
    }
}


/*
 * Convert an ISO 8859-1 stream to ASCII.
 */
static void
iso8859_1_to_ascii_mem(stream, length)
    char *stream;
    size_t length;
{
    unsigned char *strp = (unsigned char *)stream;
    size_t i;

    for (i = length; 0 < i; i--) {
	if (0x80 <= *strp)
	    *(char *)strp = iso8859_1_to_euc_table[*strp - 0x80];
	strp++;
    }
}



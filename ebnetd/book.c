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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
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

#ifdef HAVE_LIMITS_H
#include <limits.h>
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

#include "fakelog.h"
#include "permission.h"
#include "ticket.h"
#include "wildcard.h"

#include "defs.h"

/*
 * Initialize the book list `book_registry'.
 */
void
initialize_book_registry()
{
    book_registry = NULL;
    book_count = 0;
}


/*
 * Clear the book list `book_registry'.
 * Free all nodes and allocated memories in `book_registry',
 * and then set it to NULL.
 */
void
finalize_book_registry()
{
    Book *book = book_registry;
    Book *next;

    /*
     * Reset books list.  Free all malloced memories.
     */
    while (book != NULL) {
	next = book->next;
	delete_book(book);
	book = next;
    }

    book_registry = NULL;
    book_count = 0;
}


/*
 * Count books in `book_registry'.
 */
int
count_book_registry()
{
    Book *book;
    int count;

    for (book = book_registry, count = 0;
	 book != NULL; book = book->next, count++)
	;

    return count;
}


/*
 * Bind all books and appendixes in `book_registry'.
 */
int
activate_book_registry()
{
    Book *book;
    EB_Error_Code error_code;
    int available_count = 0;
    char lock_file_name[PATH_MAX + 1];

    /*
     * If there is no book in the registry, do nothing.
     */
    if (book_registry == NULL)
	return 0;

    for (book = book_registry; book != NULL; book = book->next) {
	/*
	 * Bind a book.
	 */
	error_code = eb_bind(&book->book, book->path);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_ERR, "the book is not available, %s: %s",
		eb_error_message(error_code), book->path);
	    delete_book(book);
	    continue;
	}
	available_count++;

	/*
	 * Initialize all subbooks when running as a standalone daemon.
	 */
	if (server_mode == SERVER_MODE_STANDALONE)
	    eb_load_all_subbooks(&book->book);

	/*
	 * Bind a ticket stock for the book.
	 */
	if (book->max_clients != 0) {
	    sprintf(lock_file_name, "%s/%s.%s", work_path, book->name,
		BOOK_LOCK_SUFFIX);
	    if (bind_ticket_stock(&book->ticket_stock, lock_file_name,
		book->max_clients) < 0)
		return -1;
	}

	if (book->appendix_path != NULL) {
	    /*
	     * Bind an appendix.
	     */
	    error_code = eb_bind_appendix(&book->appendix,
		book->appendix_path);
	    if (error_code != EB_SUCCESS) {
		syslog(LOG_ERR, "the appendix is not available, %s: %s",
		    eb_error_message(error_code), book->appendix_path);
		continue;
	    }

	    /*
	     * Initialize all subbooks when running as a standalone daemon.
	     */
	    if (server_mode == SERVER_MODE_STANDALONE)
		eb_load_all_appendix_subbooks(&book->appendix);
	}
    }

    return available_count;
}


/*
 * Add a new book to `book_registry'.
 * Upon successful, a pointer to the added book is returned.
 * Otherwise NULL is returned.
 */
Book *
add_book()
{
    Book *new_book;
    Book *book;
    int i;

    new_book = (Book *)malloc(sizeof(Book));
    if (new_book == NULL) {
	syslog(LOG_ERR, "memory exhausted");
	return NULL;
    }

    /*
     * Set initial values to the new book.
     */
    eb_initialize_book(&new_book->book);
    eb_initialize_appendix(&new_book->appendix);
    new_book->path = NULL;
    new_book->appendix_path = NULL;
    new_book->name[0] = '\0';
    new_book->title[0] = '\0';
    new_book->max_clients = DEFAULT_BOOK_MAX_CLIENTS;
    initialize_permission(&new_book->permissions);
    initialize_ticket_stock(&new_book->ticket_stock);
    new_book->next = NULL;

    if (book_count == 0)
	book_registry = new_book;
    else {
	book = book_registry; 
	for (i = 1; i < book_count; i++)
	    book = book->next;
	book->next = new_book;
    }

    book_count++;

    return new_book;
}


/*
 * Delete `target_book' from `book_registry'.
 * Return 0 upon success, -1 otherwise.
 */
int
delete_book(target_book)
    Book *target_book;
{
    Book *book;

    if (book_registry == target_book)
	book_registry = target_book->next;
    else {
	for (book = book_registry; book != NULL; book = book->next) {
	    if (book->next == target_book) {
		book->next = target_book->next;
		break;
	    }
	}
	if (book == NULL)
	    return -1;
    }

    finalize_permission(&target_book->permissions);
    finalize_ticket_stock(&target_book->ticket_stock);
    eb_finalize_book(&target_book->book);
    eb_finalize_appendix(&target_book->appendix);

    if (target_book->path != NULL)
	free(target_book->path);
    if (target_book->appendix_path != NULL)
	free(target_book->appendix_path);
    free(target_book);

    return 0;
}

/*
 * Find the bound book with `name' in `book_regitry'.
 *
 * If found, the pointer to the book is returned.
 * Otherwise NULL is returned.
 */
Book *
find_book(name)
    const char *name;
{
    Book *book;

    /*
     * Find the bound book.
     */
    for (book = book_registry; book != NULL; book = book->next) {
	if (strcasecmp(book->name, name) == 0)
	    return book;
    }

    return NULL;
}


/*
 * Search `book' for the subbook whose directory name is
 * `directory'.
 * If found, the subbook-code of the subbook is returned.
 * Otherwise, -1 is returned.
 */
EB_Subbook_Code
find_subbook(book, directory)
    Book *book;
    const char *directory;
{
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    char directory2[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_count;
    int i;

    /*
     * Find the subbook in the current book.
     */
    error_code = eb_subbook_list(&book->book, subbook_list, &subbook_count);
    if (error_code != EB_SUCCESS)
	return EB_SUBBOOK_INVALID;

    for (i = 0; i < subbook_count; i++) {
        error_code = eb_subbook_directory2(&book->book, subbook_list[i],
	    directory2);
        if (error_code != EB_SUCCESS)
	    continue;
	if (strcasecmp(directory, directory2) == 0)
	    return subbook_list[i];
    }

    return EB_SUBBOOK_INVALID;
}


/*
 * Search `book' for the subbook whose directory name of an appendix
 * is `directory'.
 * If found, the subbook-code of the subbook is returned.
 * Otherwise, -1 is returned.
 */
EB_Subbook_Code
find_appendix_subbook(book, directory)
    Book *book;
    const char *directory;
{
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    char directory2[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_count;
    int i;

    /*
     * Find the subbook in the current appendix.
     */
    error_code = eb_appendix_subbook_list(&book->appendix, subbook_list,
	&subbook_count);
    if (error_code != EB_SUCCESS)
	return EB_SUBBOOK_INVALID;

    for (i = 0; i < subbook_count; i++) {
        error_code = eb_appendix_subbook_directory2(&book->appendix,
	    subbook_list[i], directory2);
        if (error_code != EB_SUCCESS)
	    continue;
        if (strcasecmp(directory, directory2) == 0)
	    return subbook_list[i];
    }

    return EB_SUBBOOK_INVALID;
}


/*
 * For the current client,
 * check permission flags to the books in `book_registry'.
 */
void
check_book_permissions()
{
    Book *book;

    for (book = book_registry; book != NULL; book = book->next) {
	book->permission_flag = test_permission(&book->permissions,
	    client_host_name, client_address, match_wildcard);
    }
}


/*
 * For the current client,
 * set permission flags to all books in `book_registry'.
 */
void
set_all_book_permissions()
{
    Book *book;

    for (book = book_registry; book != NULL; book = book->next)
	book->permission_flag = 1;
}



/*
 * Copyright (c) 2003
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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#include "ebtiny/eb.h"
#include "ebtiny/error.h"


/*
 * Unexported functions.
 */
static EB_Error_Code eb_load_catalog_eb EB_P((EB_Book *, const char *));
static EB_Error_Code eb_load_catalog_epwing EB_P((EB_Book *, const char *));


void
eb_initialize_book(book)
    EB_Book *book;
{
    int i;

    book->subbook_count = 0;
    for (i = 0; i < book->subbook_count; i++)
	book->subbooks[i][0] = '\0';
}


void
eb_finalize_book(book)
    EB_Book *book;
{
    int i;

    for (i = 0; i < book->subbook_count; i++)
	book->subbooks[i][0] = '\0';
    book->subbook_count = 0;
}


EB_Error_Code
eb_load_all_subbooks(book)
    EB_Book *book;
{
    return EB_SUCCESS;
}


EB_Error_Code
eb_bind(book, path)
    EB_Book *book;
    const char *path;
{
    EB_Error_Code error_code;
    size_t path_length;

    if (book->subbooks != NULL) {
        eb_finalize_book(book);
        eb_initialize_book(book);
    }

    if (*path != '/')
	return EB_ERR_BAD_FILE_NAME;

    path_length = strlen(path);
    if (PATH_MAX < path_length + 1 + EB_MAX_FILE_NAME_LENGTH)
        return EB_ERR_TOO_LONG_FILE_NAME;

    error_code = eb_load_catalog_eb(book, path);
    if (error_code != EB_SUCCESS) 
	error_code = eb_load_catalog_epwing(book, path);
    if (error_code != EB_SUCCESS)
	return error_code;

    return EB_SUCCESS;
}


static EB_Error_Code
eb_load_catalog_eb(book, book_path)
    EB_Book *book;
    const char *book_path;
{
    char catalog_path_name[PATH_MAX + 1];
    char catalog_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char buffer[40];
    char *space;
    EB_Error_Code error_code;
    int file = -1;
    int i;

    /*
     * Open a catalog file.
     */
    error_code = eb_find_file_name(book_path, "catalog",
	catalog_file_name);
    if (error_code != EB_SUCCESS)
        goto failed;

    sprintf(catalog_path_name, "%s/%s", book_path, catalog_file_name);
    file = open(catalog_path_name, O_RDONLY);
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_CAT;
	goto failed;
    }

    /*
     * Get the number of subbooks in this book.
     */
    if (eb_read_all(file, buffer, 16) != 16) {
        error_code = EB_ERR_FAIL_READ_CAT;
        goto failed;
    }

    book->subbook_count = (*(unsigned char *)buffer << 8)
	+ (*(unsigned char *)(buffer + 1));
    if (EB_MAX_SUBBOOKS < book->subbook_count)
	book->subbook_count = EB_MAX_SUBBOOKS;
    if (book->subbook_count == 0) {
	error_code = EB_ERR_UNEXP_CAT;
	goto failed;
    }

    /*
     * Read subbook information.
     */
    for (i = 0; i < book->subbook_count; i++) {
        if (eb_read_all(file, buffer, 40) != 40) {
	    error_code = EB_ERR_FAIL_READ_CAT;
	    goto failed;
	}
	strncpy(book->subbooks[i], buffer + 32,
	    EB_MAX_DIRECTORY_NAME_LENGTH);
	*(book->subbooks[i] + EB_MAX_DIRECTORY_NAME_LENGTH) = '\0';
        space = strchr(book->subbooks[i], ' ');
        if (space != NULL)
            *space = '\0';
    }

    close(file);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);

    return error_code;
}


static EB_Error_Code
eb_load_catalog_epwing(book, book_path)
    EB_Book *book;
    const char *book_path;
{
    char catalog_path_name[PATH_MAX + 1];
    char catalog_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char buffer[164];
    char *space;
    EB_Error_Code error_code;
    int file = -1;
    int i;

    /*
     * Open a catalog file.
     */
    error_code = eb_find_file_name(book_path, "catalogs",
	catalog_file_name);
    if (error_code != EB_SUCCESS)
        goto failed;

    sprintf(catalog_path_name, "%s/%s", book_path, catalog_file_name);
    file = open(catalog_path_name, O_RDONLY);
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_CAT;
	goto failed;
    }

    /*
     * Get the number of subbooks in this book.
     */
    if (eb_read_all(file, buffer, 16) != 16) {
        error_code = EB_ERR_FAIL_READ_CAT;
        goto failed;
    }

    book->subbook_count = (*(unsigned char *)buffer << 8)
	+ (*(unsigned char *)(buffer + 1));
    if (EB_MAX_SUBBOOKS < book->subbook_count)
	book->subbook_count = EB_MAX_SUBBOOKS;
    if (book->subbook_count == 0) {
	error_code = EB_ERR_UNEXP_CAT;
	goto failed;
    }

    /*
     * Read subbook information.
     */
    for (i = 0; i < book->subbook_count; i++) {
        if (eb_read_all(file, buffer, 164) != 164) {
	    error_code = EB_ERR_FAIL_READ_CAT;
	    goto failed;
	}
	strncpy(book->subbooks[i], buffer + 82,
	    EB_MAX_DIRECTORY_NAME_LENGTH);
	*(book->subbooks[i] + EB_MAX_DIRECTORY_NAME_LENGTH) = '\0';
        space = strchr(book->subbooks[i], ' ');
        if (space != NULL)
            *space = '\0';
    }

    close(file);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);

    return error_code;
}


EB_Error_Code
eb_subbook_list(book, subbook_list, subbook_count)
    EB_Book *book;
    EB_Subbook_Code *subbook_list;
    int *subbook_count;
{
    int i;

    if (book->subbooks == NULL)
	return EB_ERR_UNBOUND_BOOK;

    for (i = 0; i < book->subbook_count; i++)
	subbook_list[i] = i;
    *subbook_count = book->subbook_count;

    return EB_SUCCESS;
}


EB_Error_Code
eb_subbook_directory2(book, subbook_code, directory)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
    char *directory;
{
    char *p;

    if (book->subbooks == NULL)
	return EB_ERR_UNBOUND_BOOK;

    if (subbook_code < 0 || book->subbook_count <= subbook_code)
        return EB_ERR_NO_SUCH_SUB;

    strcpy(directory, book->subbooks[subbook_code]);
    for (p = directory; *p != '\0'; p++) {
        if ('A' <= *p && *p <= 'Z')
            *p += ('a' - 'A');
    }

    return EB_SUCCESS;
}


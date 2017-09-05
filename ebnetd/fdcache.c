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
#include <sys/types.h>
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

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "fdcache.h"

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
 * Unexported functions.
 */
#ifdef __STDC__
static unsigned int fdcache_compute_path_hash(const char *);
static FDCache_Entry *fdcache_create_entry(const char *, int);
static void fdcache_destroy_entry(FDCache_Entry *);
static FDCache_Entry *fdcache_find_entry(FDCache_Table *, const char *);
static void fdcache_insert_entry(FDCache_Table *, FDCache_Entry *);
static void fdcache_purge_entry(FDCache_Table *, FDCache_Entry *);
#else
static unsigned int fdcache_compute_path_hash();
static FDCache_Entry *fdcache_create_entry();
static void fdcache_destroy_entry();
static FDCache_Entry *fdcache_find_entry();
static void fdcache_insert_entry();
static void fdcache_purge_entry();
#endif


/*
 * Compute hash value for `path'.
 */
static unsigned int
fdcache_compute_path_hash(path)
    const char *path;
{
    const unsigned char *p = (const unsigned char *)path;
    unsigned int value = 0;

    while (*p != '\0')
	value = (value << 3) + value + *p++;

    return value;
}


/*
 * Create a new cache entry.
 */
static FDCache_Entry *
fdcache_create_entry(path, fd)
    const char *path;
    int fd;
{
    FDCache_Entry *new_entry;
    char cwd[PATH_MAX + 1];
    int saved_errno;

    new_entry = (FDCache_Entry *)malloc(sizeof(FDCache_Entry));
    if (new_entry == NULL)
	goto failed;
    new_entry->fd = fd;
    new_entry->path_hash = fdcache_compute_path_hash(path);

    if (*path == '/') {
	new_entry->path = malloc(strlen(path) + 1);
	if (new_entry->path == NULL)
	    goto failed;
	strcpy(new_entry->path, path);
    } else {
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    goto failed;
	new_entry->path = malloc(strlen(cwd) + 1 + strlen(path) + 1);
	if (new_entry->path == NULL)
	    goto failed;
	sprintf("%s/%s", cwd, path);
    }

    return new_entry;

  failed:
    saved_errno = errno;
    if (new_entry != NULL)
	fdcache_destroy_entry(new_entry);
    errno = saved_errno;

    return NULL;
}


/*
 * Destroy an entry created by fdcache_create_entry().
 */
static void
fdcache_destroy_entry(entry)
    FDCache_Entry *entry;
{
    if (entry->path != NULL)
	free(entry->path);
    free(entry);
}


/*
 * Search the cache table for an entry with `path'.
 */
static FDCache_Entry *
fdcache_find_entry(table, path)
    FDCache_Table *table;
    const char *path;
{
    FDCache_Entry *entry;
    unsigned int path_hash;

    path_hash = fdcache_compute_path_hash(path);

    for (entry = table->head; entry != NULL; entry = entry->next) {
	if (entry->path_hash == path_hash && strcmp(entry->path, path) == 0)
	    return entry;
    }

    return NULL;
}


/*
 * Insert `entry' into the head of the cache table.
 */
static void
fdcache_insert_entry(table, entry)
    FDCache_Table *table;
    FDCache_Entry *entry;
{
    if (table->entry_count == 0) {
	table->tail = entry;
	table->head = entry;
	entry->next = NULL;
	entry->back = NULL;
    } else {
	entry->next = table->head;
	table->head->back = entry;
	entry->back = NULL;
	table->head = entry;
    }

    table->entry_count++;
}


/*
 * Delete `entry' from the cache table.
 */
static void
fdcache_purge_entry(table, entry)
    FDCache_Table *table;
    FDCache_Entry *entry;
{
    if (entry->next != NULL)
	entry->next->back = entry->back;
    if (entry->back != NULL)
	entry->back->next = entry->next;
    if (entry == table->tail)
	table->tail = entry->back;
    if (entry == table->head)
	table->head = entry->next;

    table->entry_count--;
}


/*
 * Initialize the cache table.
 */
void
#ifdef __STDC__
fdcache_initialize_table(FDCache_Table *table, int flags, ...)
#else
fdcache_initialize_table(table, flags, va_alist)
    FDCache_Table *table;
    int flags;
    va_dcl
#endif
{
    va_list ap;
    mode_t mode;

    if ((flags & O_CREAT) == 0)
	mode = 0;
    else {
#ifdef __STDC__
	va_start(ap, flags);
#else
	va_start(ap);
#endif
	mode = va_arg(ap, int);
	va_end(ap);
    }

    table->head = NULL;
    table->tail = NULL;
    table->entry_count = 0;
    table->open_flags = flags;
    table->open_mode = mode;
    table->max_entry_count = 0;
}


/*
 * Finalize the cache table.
 * Close all file descritors in the cache Table.
 */
void
fdcache_finalize_table(table)
    FDCache_Table *table;
{
    FDCache_Entry *entry;

    for (;;) {
	entry = table->head;
	if (entry == NULL)
	    break;
	close(entry->fd);
	fdcache_purge_entry(table, entry);
	fdcache_destroy_entry(entry);
    }
}


/*
 * Set `max_entry_count'.
 */
void
fdcache_set_max_entry_count(table, count)
    FDCache_Table *table;
    int count;
{
    table->max_entry_count = count;
}


/*
 * Return `max_entry_count'.
 */
int
fdcache_max_entry_count(table)
    FDCache_Table *table;
{
    return table->max_entry_count;
}


/*
 * Return `entry_count'.
 */
int
fdcache_entry_count(table)
    FDCache_Table *table;
{
    return table->entry_count;
}


/*
 * Open the file specified by `path'.
 */
int
fdcache_open(table, path)
    FDCache_Table *table;
    const char *path;
{
    FDCache_Entry *entry;
    FDCache_Entry *lru_entry;
    int saved_errno;
    int fd;

    entry = fdcache_find_entry(table, path);
    if (entry != NULL) {
	fdcache_purge_entry(table, entry);
	fdcache_insert_entry(table, entry);
	return entry->fd;
    }

    /*
     * Open the file.
     */
    if ((table->open_flags & O_CREAT) == 0)
	fd = open(path, table->open_flags);
    else
	fd = open(path, table->open_flags, table->open_mode);
    if (fd < 0)
	return -1;

    /*
     * Create a cache entry and insert it into the cache table.
     */
    entry = fdcache_create_entry(path, fd);
    if (entry == NULL) {
	saved_errno = errno;
	close(fd);
	errno = saved_errno;
	return -1;
    }
    fdcache_insert_entry(table, entry);

    /*
     * Delete an LRU (least recently used) entry.
     */
    if (0 < table->max_entry_count 
	&& table->max_entry_count < table->entry_count) {
	lru_entry = table->tail;
	close(lru_entry->fd);
	fdcache_purge_entry(table, lru_entry);
	fdcache_destroy_entry(lru_entry);
    }

    return entry->fd;
}

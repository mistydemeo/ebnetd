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

#ifndef FDCACHE_H
#define FDCACHE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

/*
 * File cache entry.
 */
typedef struct FDCache_Entry_Struct FDCache_Entry;
struct FDCache_Entry_Struct {
    /*
     * File Descriptor
     */
    int fd;

    /*
     * File name.
     */
    char *path;

    /*
     * Hash value of `path'.
     */
    unsigned int path_hash;

    /*
     * Next and previous entry.
     */
    FDCache_Entry *next;
    FDCache_Entry *back;
};

/*
 * File cache table.
 */
typedef struct FDCache_Table_Struct FDCache_Table;
struct FDCache_Table_Struct {
    /*
     * First entry.
     */
    FDCache_Entry *head;

    /*
     * Last entry.
     */
    FDCache_Entry *tail;

    /*
     * The number of entries in the cache table.
     */
    int entry_count;

    /*
     * The maximum number of entries in the cache table.
     */
    int max_entry_count;

    /*
     * flags and mode for open().
     */
    int open_flags;
    mode_t open_mode;
};

#ifdef __STDC__
void fdcache_initialize_table(FDCache_Table *, int, ...);
void fdcache_finalize_table(FDCache_Table *);
void fdcache_set_max_entry_count(FDCache_Table *, int);
int fdcache_max_entry_count(FDCache_Table *);
int fdcache_entry_count(FDCache_Table *);
int fdcache_open(FDCache_Table *, const char *);
#else
void fdcache_initialize_table();
void fdcache_finalize_table();
void fdcache_set_max_entry_count();
int fdcache_max_entry_count();
int fdcache_entry_count();
int fdcache_open();
#endif

#endif /* FDCACHE_H */


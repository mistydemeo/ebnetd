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

#ifndef READCONF_H
#define READCONF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Maximum Line length of a configuration file.
 */
#define READCONF_SIZE_LINE		511

/*
 * Maximum Block name in a `Config' structure.
 */
#define READCONF_SIZE_BLOCK_NAME	127

/*
 * Definition type macros.
 */
#define READCONF_ZERO_OR_ONCE		0
#define READCONF_ONCE			1
#define READCONF_ZERO_OR_MORE		2
#define READCONF_ONCE_OR_MORE		3

/*
 * Configuration syntax table.
 */
typedef struct {
    /*
     * Block name.
     */
    char *block;

    /*
     * Directive name.
     */
    char *name;

    /*
     * Function to process the directive.
     */
#ifdef __STDC__
    int (*function)(const char *, const char *, int);
#else
    int (*function)();
#endif

    /*
     * Definititon type.
     */
    int type;

    /*
     * Directive definition counter.
     * This counter is managed by readconf module.  Don't modify
     * the counter in elsewhere.
     */
    int count;
} Configuration;


#ifdef __STDC__
int read_configuration(const char *filename, Configuration *);
#else /* not __STDC__ */
int read_configuration();
#endif /* not __STDC__ */

#endif /* not READCONF_H */

/*
 * Copyright (c) 2000, 01
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

#ifndef PATHPARTS_H
#define PATHPARTS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Segments in a path.
 */
typedef struct {
    int is_absolute;		/* true if this is absolute path */
    char **segments;		/* segment list */
    int segment_count;		/* the number of segments */
    char *leaf;			/* leaf path of a path */
    char *buffer;		/* buffer of `segments' */
} Path_Parts;


/*
 * Function Declarations.
 */
#ifdef __STDC__
void path_parts_initialize(Path_Parts *);
void path_parts_finalize(Path_Parts *);
int path_parts_parse(Path_Parts *, const char *);
int path_parts_segment_count(Path_Parts *);
const char *path_parts_leaf(Path_Parts *);
const char *path_parts_segment(Path_Parts *, int);
int path_parts_is_absolute(Path_Parts *);
int path_parts_is_relative(Path_Parts *);
#else
void path_parts_initialize();
void path_parts_finalize();
int path_parts_parse();
int path_parts_segment_count();
const char *path_parts_segment();
const char *path_parts_leaf();
int path_parts_is_absolute();
int path_parts_is_relative();
#endif

#endif /* not PATHPARTS_H */

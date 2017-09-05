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

#include "pathparts.h"

/*
 * Initialize `path_segments'.
 */
void
path_parts_initialize(path_parts)
    Path_Parts *path_parts;
{
    path_parts->is_absolute = 0;
    path_parts->segment_count = 0;
    path_parts->segments = NULL;
    path_parts->leaf = 0;
    path_parts->buffer = NULL;
}

/*
 * Finalize `path_parts'.
 */
void
path_parts_finalize(path_parts)
    Path_Parts *path_parts;
{
    if (path_parts->segments != NULL) {
	free(path_parts->segments);
	path_parts->segments = NULL;
    }

    path_parts->is_absolute = 0;
    path_parts->segment_count = 0;
    path_parts->leaf = 0;

    if (path_parts->buffer != NULL) {
	free(path_parts->buffer);
	path_parts->buffer = NULL;
    }
}

/*
 * Split a `path' into `path_parts' an a list of segments.
 * If succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
path_parts_parse(path_parts, path)
    Path_Parts *path_parts;
    const char *path;
{
    char *buffer_p;
    size_t buffer_length;
    const char *first_segment;
    int i;

    /*
     * Test whether `path' is absolute or relative path.
     */
    if (*path == '/') {
	path_parts->is_absolute = 1;
	first_segment = path + 1;
    } else {
	path_parts->is_absolute = 0;
	first_segment = path;
    }

    /*
     * Re-initialize `path_parts' if it has already been used.
     */
    if (path_parts->buffer != NULL
	|| path_parts->segments != NULL) {
	path_parts_finalize(path_parts);
	path_parts_initialize(path_parts);
    }

    /*
     * Allocate memories to `path_parts->buffer', and copy `path'
     * to there.  However, a slash (`/') at the beginning of `path'
     * is not copied.
     */
    buffer_length = strlen(first_segment) + 1;
    path_parts->buffer = (char *) malloc(buffer_length);
    if (path_parts->buffer == NULL)
	goto failed;
    memcpy(path_parts->buffer, first_segment, buffer_length);

    /*
     * Count the number of slashes (`/') in `path_parts->buffer'.
     */
    buffer_p = path_parts->buffer;
    path_parts->segment_count = 0;
    while (*buffer_p != '\0') {
	if (*buffer_p == '/') {
	    path_parts->segment_count++;
	}
	buffer_p++;
    }

    /*
     * Allocate memory for `path_parts->segments'.
     */
    path_parts->segments
	= (char **) malloc(sizeof(char *) * path_parts->segment_count + 1);
    if (path_parts->segments == NULL)
	goto failed;

    /*
     * Split `path_parts->buffer' with a separator `/'.
     */
    buffer_p = path_parts->buffer;
    for (i = 0; i < path_parts->segment_count; i++) {
	*(path_parts->segments + i) = buffer_p;
	while (*buffer_p != '/' && *buffer_p != '\0')
	    buffer_p++;
	*buffer_p = '\0';
	buffer_p++;
    }
    *(path_parts->segments + i) = NULL;
    
    /*
     * Set leaf, if exists.
     */
    if (buffer_p - path_parts->buffer + 1 < buffer_length)
	path_parts->leaf = buffer_p;

    return 0;

    /*
     * If an error occurs...
     */
  failed:
    path_parts_initialize(path_parts);
    return -1;
}


/*
 * Return the number of segments in a path parsed by `path_parts'
 * in previous.
 */
int
path_parts_segment_count(path_parts)
    Path_Parts *path_parts;
{
    return path_parts->segment_count;
}


/*
 * Return an `n'th segments of a path parsed by `path_parts'
 * in previous.  NULL is returned if there is no `n'th segments in 
 * the path or `n' is negative.
 */
const char *
path_parts_segment(path_parts, n)
    Path_Parts *path_parts;
    int n;
{
    if (n < 0 || path_parts->segment_count <= n)
	return NULL;

    return *(path_parts->segments + n);
}


/*
 * Return a leaf part of a path parsed by `path_parts' in previous.
 * If the path lacks a leaf, NULL is returned.
 */
const char *
path_parts_leaf(path_parts)
    Path_Parts *path_parts;
{
    return path_parts->leaf;
}


/*
 * Test whether a path parsed by `path_parts' in previous is absolute
 * path.  It returns 1 if it is, and 0 otherwise.
 */
int
path_parts_is_absolute(path_parts)
    Path_Parts *path_parts;
{
    return path_parts->is_absolute;
}


/*
 * Test whether a path parsed by `path_parts' in previous is relative
 * path.  It returns 1 if it is, and 0 otherwise.
 */
int
path_parts_is_relative(path_parts)
    Path_Parts *path_parts;
{
    return !(path_parts->is_absolute);
}


#ifdef TEST
int
main(argc, argv)
    int argc;
    char *argv[];
{
    Path_Parts path_parts;
    int segment_count;
    const char *segment;
    const char *leaf;
    int i;

    path_parts_initialize(&path_parts);

    if (argc != 2) {
	fprintf(stderr, "usage: %s path\n", argv[0]);
	exit(1);
    }

    if (path_parts_parse(&path_parts, argv[1]) < 0) {
	fprintf(stderr, "%s: bad path\n", argv[0]);
	exit(1);
    }

    segment_count = path_parts_segment_count(&path_parts);
    for (i = 0; i < segment_count; i++) {
	segment = path_parts_segment(&path_parts, i);
	printf("segments %d: %s\n", i, segment);
    }

    leaf = path_parts_leaf(&path_parts);
    if (leaf != NULL)
	printf("leaf: %s\n", leaf);

    path_parts_finalize(&path_parts);
}

#endif

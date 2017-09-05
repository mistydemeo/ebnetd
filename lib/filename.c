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

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, unistd.h, limits.h)
 *   AC_CHECK_FUNCS(strchr, memcpy, getcwd)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <syslog.h>

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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

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

#ifndef HAVE_GETCWD
#define getcwd(d,n) getwd(d)
#endif

/*
 * The maximum length of a file name.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif


/*
 * Canonicalize `path_name'.
 * Convert a path name to an absolute path.
 * Return 0 upon success, -1 otherwise.
 */
int
canonicalize_file_name(path_name)
    char *path_name;
{
    char cwd[PATH_MAX + 1];
    char tmp_path_name[PATH_MAX + 1];
    size_t path_name_length;

    if (*path_name != '/') {
	/*
	 * `path_name' is an relative path.  Convert to an absolute
	 * path.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return -1;
	if (PATH_MAX < strlen(cwd) + 1 + strlen(path_name))
	    return -1;
	sprintf(tmp_path_name, "%s/%s", cwd, path_name);
	strcpy(path_name, tmp_path_name);
    }

    /*
     * Unless `path_name' is "/", eliminate `/' in the tail of the
     * path name.
     */
    path_name_length = strlen(path_name);
    if (1 < path_name_length && *(path_name + path_name_length - 1) == '/')
	*(path_name + path_name_length - 1) = '\0';

    return 0;
}

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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

/*
 * Do wildcard pattern matching.
 * In the pattern, the following characters have special meaning.
 * 
 *   `*'    matches any sequence of zero or more characters.
 *   '\x'   a character following a backslash is taken literally.
 *          (e.g. '\*' means an asterisk itself.)
 *
 * If `pattern' matches to `string', 1 is returned.  Otherwise 0 is
 * returned.
 */
int
match_wildcard(pattern, string)
    const char *pattern;
    const char *string;
{
    const char *pattern_p = pattern;
    const char *string_p = string;

    while (*pattern_p != '\0') {
	if (*pattern_p == '*') {
	    pattern_p++;
	    if (*pattern_p == '\0')
		return 1;
	    while (*string_p != '\0') {
		if (*string_p == *pattern_p
		    && match_wildcard(pattern_p, string_p))
		    return 1;
		string_p++;
	    }
	    return 0;
	} else {
	    if (*pattern_p == '\\' && *(pattern_p + 1) != '\0')
		pattern_p++;
	    if (*pattern_p != *string_p)
		return 0;
	}
	pattern_p++;
	string_p++;
    }

    return (*string_p == '\0');
}



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

#ifndef HTTPDATE_H
#define HTTPDATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

/*
 * The length of an HTTP date (e.g. "Tue, 25 Jun 1997 03:57:33 GMT").
 */
#define HTTP_DATE_STRING_LENGTH		30

/*
 * Function Declarations.
 */
#ifdef __STDC__
int http_make_date(char *);
int http_make_date2(char *, const struct tm *);
int http_compare_date(struct tm *, struct tm *);
int http_parse_date(const char *, struct tm *);
void http_set_origin_date(struct tm *);
#else
int http_make_date();
int http_make_date2();
int http_compare_date();
int http_parse_date();
void http_set_origin_date();
#endif

#endif /* not HTTPDATE_H */

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

#ifndef CGIQUERY_H
#define CGIQUERY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * A `name=value' pair in CGI Query
 */
typedef struct CGI_Query_Element_Struct CGI_Query_Element;

struct CGI_Query_Element_Struct {
    char *name;
    char *value;
    CGI_Query_Element *next;
};

/*
 * CGI Query.
 */
typedef struct {
    int element_count;
    CGI_Query_Element *elements;
    char *elements_buffer;
} CGI_Query;


/*
 * Function Declarations.
 */
#ifdef __STDC__
void cgi_query_initialize(CGI_Query *);
void cgi_query_finalize(CGI_Query *);
const char *cgi_query_element_value(CGI_Query *, const char *);
const char *cgi_query_element_string_value(CGI_Query *, const char *);
int cgi_query_element_integer_value(CGI_Query *, const char *);
int cgi_query_parse_string(CGI_Query *, const char *);
#else /* not __STDC__ */
void cgi_query_initialize();
void cgi_query_finalize();
const char *cgi_query_element_value();
const char *cgi_query_element_string_value();
const char *cgi_query_element_integer_value();
int cgi_query_parse_string();
#endif /* not __STDC__ */

#endif /* not CGIQUERY_H */

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

#ifndef HAVE_STRTOL
#ifdef __STDC__
long strtol(const char *, char **, int);
#else /* not __STDC__ */
long strtol();
#endif /* not __STDC__ */
#endif /* not HAVE_STRTOL */

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp()
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define isupper(c) ('A' <= (c) && (c) <= 'Z')
#define islower(c) ('a' <= (c) && (c) <= 'z')
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isupper(c) || islower(c) || isdigit(c))
#define isxdigit(c) \
  (isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define toupper(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define tolower(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))


#include "cgiquery.h"

/*
 * Unexported functions.
 */
#ifdef __STDC__
static void cgi_query_decode_string(char *);
#else
static void cgi_query_decode_string();
#endif

/*
 * Initialize a query.
 */
void
cgi_query_initialize(query)
    CGI_Query *query;
{
    query->element_count = 0;
    query->elements = NULL;
    query->elements_buffer = NULL;
}


/*
 * Finalize a query.
 * All allocated memories in `queue' are also disposed.
 */
void
cgi_query_finalize(query)
    CGI_Query *query;
{
    CGI_Query_Element *element;
    CGI_Query_Element *element_next;

    /*
     * Dispose all elements in a query.
     */
    if (query->elements != NULL) {
	element = query->elements;
	while (element != NULL) {
	    element_next = element->next;
	    free(element);
	    element = element_next;
	}
	query->elements = NULL;

	if (query->elements_buffer != NULL)
	    free(query->elements_buffer);
	query->elements_buffer = NULL;
    }

    /*
     * Set element count to 0.
     */
    query->element_count = 0;
}


/*
 * Find a query element whose name is `name'.
 * If the element is found, a given value for the name is returned.
 * Otherwise, NULL is returned.
 */
const char *
cgi_query_element_value(query, name)
    CGI_Query *query;
    const char *name;
{
    CGI_Query_Element *element;

    element = query->elements;
    while (element != NULL) {
	if (strcmp(element->name, name) == 0)
	    return (const char *)element->value;
	element = element->next;
    }

    return NULL;
}


/*
 * This function is equevalent to cgi_query_element_value(), except
 * that it returns an empty string when `name' is not defined.
 */
const char *
cgi_query_element_string_value(query, name)
    CGI_Query *query;
    const char *name;
{
    CGI_Query_Element *element;

    element = query->elements;
    while (element != NULL) {
	if (strcmp(element->name, name) == 0)
	    return (const char *)element->value;
	element = element->next;
    }

    return "";
}


/*
 * Find a query element whose name is `name'.
 * If the element is found, this function converts the string value
 * for the name to an integer and then returns the value.
 * Otherwise, 0 is returned.
 */
int
cgi_query_element_integer_value(query, name)
    CGI_Query *query;
    const char *name;
{
    CGI_Query_Element *element;

    element = query->elements;
    while (element != NULL) {
	if (strcmp(element->name, name) == 0) {
	    if (strncasecmp(element->value, "0x", 2) == 0)
		return strtol((const char *)element->value + 2, NULL, 16);
	    else
		return strtol((const char *)element->value, NULL, 10);
	}
	element = element->next;
    }

    return 0;
}


/*
 * Parse a query string.
 * The argument `query_string' is a CGI query.  That is a list of
 * `name' or `name=value' elements, separated by an ampersand (`&'),
 * like this:
 *
 *    "word=hyper+text&sort=score&maxhits=20"
 *
 * Note that if an element has a name only, the value for the element
 * is set to an empty string, not NULL.
 *
 * This function returns -1 if not enough memory is remained, 
 * returns 0 otherwise.
 */
int
cgi_query_parse_string(query, query_string)
    CGI_Query *query;
    const char *query_string;
{
    CGI_Query_Element *element;
    char *query_string_p;
    char *name;
    char *value;
    size_t query_string_length;
    int end_of_query_string;

    /*
     * Re-initialize if `query' has already been used.
     */
    if (query->elements != NULL) {
	cgi_query_finalize(query);
	cgi_query_initialize(query);
    }

    /*
     * Copy `query_string' to `query->elements_buffer'.
     */
    query_string_length = strlen(query_string);
    query->elements_buffer = (char *)malloc(query_string_length + 1);
    if (query->elements_buffer == NULL)
	goto failed;
    memcpy(query->elements_buffer, query_string, query_string_length + 1);

    /*
     * Parse `query_string'.
     */
    query_string_p = query->elements_buffer;
    name = query->elements_buffer;
    value = NULL;
    end_of_query_string = 0;

    for (;;) {
	if (*query_string_p == '=' && value == NULL) {
	    /*
	     * `=' separates name and value.
	     */
	    *query_string_p = '\0';
	    value = query_string_p + 1;

	} else if (*query_string_p == '&' || *query_string_p == '\0') {
	    /*
	     * `&' or `\0' terminates a query element.
	     */
	    if (*query_string_p == '\0')
		end_of_query_string = 1;
	    *query_string_p = '\0';

	    /*
	     * Allocate memories for the element, and set name and value
	     * of the element.
	     */
	    element = (CGI_Query_Element *)
		malloc(sizeof(CGI_Query_Element));
	    if (element == NULL)
		goto failed;

	    element->name = name;
	    if (value == NULL)
		element->value = query_string_p;
	    else
		element->value = value;

	    /*
	     * Decode name and value.
	     */
	    cgi_query_decode_string(element->name);
	    cgi_query_decode_string(element->value);

	    element->next = query->elements;
	    query->elements = element;
	    query->element_count++;

	    /*
	     * Exit the loop if `\0' is appeared.
	     */
	    if (end_of_query_string)
		break;

	    name = query_string_p + 1;
	    value = NULL;
	}
	query_string_p++;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    cgi_query_finalize(query);
    return -1;
}


/*      
 * Decode all "%<HEX><HEX>" and "+" in a query name or value.
 * "%<HEX><HEX>" is decoded to an 8 bits code <HEX><HEX>.
 * "+" is decoded to a space.
 */
static void
cgi_query_decode_string(string)
    char *string;
{
    char *source_string_p = string;
    char *destination_string_p = string;
    int hex1;
    int hex2;
    int c;

    while (*source_string_p != '\0') {
	if (*source_string_p == '%'
	    && isxdigit(*(source_string_p + 1))
	    && isxdigit(*(source_string_p + 2))) {
	    /*
	     * "%<HEX><HEX>" occurs.
	     * Convert it to an 8 bit code <HEX><HEX>.
	     */
	    hex1 = *(source_string_p + 1);
	    hex2 = *(source_string_p + 2);
	    c = 0;

	    if ('0' <= hex1 && hex1 <= '9')
		c += (hex1 - '0') << 4;
	    else if ('A' <= hex1 && hex1 <= 'F')
		c += (hex1 - 'A' + 0x0a) << 4;
	    else if ('a' <= hex1 && hex1 <= 'f')
		c += (hex1 - 'a' + 0x0a) << 4;

	    if ('0' <= hex2 && hex2 <= '9')
		c += (hex2 - '0');
	    else if ('A' <= hex2 && hex2 <= 'F')
		c += (hex2 - 'A' + 0x0a);
	    else if ('a' <= hex2 && hex2 <= 'f')
		c += (hex2 - 'a' + 0x0a);

	    *(unsigned char *)destination_string_p = c;
	    source_string_p += 3;
	    destination_string_p++;

	} else if (*source_string_p == '+') {
	    /*
	     * `+' occurs.
	     * Convert it to a space.
	     */
	    *destination_string_p = ' ';
	    source_string_p++;
	    destination_string_p++;

	} else {
	    *destination_string_p = *source_string_p;
	    source_string_p++;
	    destination_string_p++;
	}
    }

    *destination_string_p = '\0';
}


/*
 * Print a query state to standard out, like this:
 *
 *   cgi_query = {
 *     element_count = 3,
 *     elements = {
 *      {name = word, value = hyper text},
 *      {name = sort, value = score},
 *      {name = maxhits, value = 20}
 *     }
 *   }
 */
#if defined(DEBUG) || defined(TEST)
void
cgi_query_print(query)
    CGI_Query *query;
{
    CGI_Query_Element *element;
    char *name_p;

    /*
     * Print header and element_count.
     */
    printf("cgi_query = {\n element_count = %d,\n elements = {\n",
	query->element_count);

    for (element = query->elements; element != NULL; element = element->next) {
	/*
	 * Print name.
	 */
	fputs("  {name = \"", stdout);
	for (name_p = element->name; *name_p != '\0'; name_p++) {
	    if (' ' <= *name_p && *name_p <= '~' && *name_p != '"')
		fputc(*name_p, stdout);
	    else
		printf("\\x%02x", *(unsigned char *)name_p);
	}

	/*
	 * Print value.
	 */
	fputs("\", value = \"", stdout);
	for (name_p = element->value; *name_p != '\0'; name_p++) {
	    if (' ' <= *name_p && *name_p <= '~' && *name_p != '"')
		fputc(*name_p, stdout);
	    else
		printf("\\x%02x", *(unsigned char *)name_p);
	}

	/*
	 * Print footer.
	 */
	if (element->next == NULL)
	    fputs("\"}\n", stdout);
	else
	    fputs("\"},\n", stdout);
    }

    /*
     * Print footer.
     */
    fputs("}\n", stdout);
    fflush(stdout);
}
#endif /* defined(DEBUG) || defined(TEST) */


/*
 * Main for test.
 */
#ifdef TEST

int
main(argc, argv)
    int argc;
    char *argv[];
{
    CGI_Query query;

    cgi_query_initialize(&query);

    if (argc < 2) {
	fprintf(stderr, "usage: %s query-string\n", argv[0]);
	exit(1);
    }

    cgi_query_parse_string(&query, argv[1]);
    cgi_query_print(&query);
}

#endif

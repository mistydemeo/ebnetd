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

#include "httpdate.h"

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
#endif /* not HAVE_MEMCPY */

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp();
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


/*
 * Abbreviated months names.
 */
static const char *abbreviated_months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
    "Nov", "Dec"
};

/*
 * Abbreviated weekdays names.
 */
static const char *abbreviated_weekdays[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/*
 * Full weekdays names.
 */
static const char *full_weekdays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
    "Saturday"
};

/*
 * String length of full weekdays names.
 * `Full_weekdays_len[i]' is equal to `strlen(full_weekdays[i])'.
 */
static const int full_weekdays_length[] = {6, 6, 7, 9, 8, 6, 8};

/*
 * Unexported function.
 */
#ifdef __STDC__
static int http_parse_date_internal(struct tm *, const char *, const char *);
#else
static int http_parse_date_internal();
#endif


/*
 * Generate currnet date string like this:
 *
 * 	    "Tue, 25 Jun 1997 03:57:33 GMT"
 *
 * The format of date is described in RFC 1123.
 * `string' must refer to the buffer with 30 bytes or larger.
 * If succeeds to generate, 0 is returned.  Otherwise -1 is returned.
 */
int
http_make_date(string)
    char *string;
{
    time_t t;
    struct tm *gmt;

    if (time(&t) == (time_t)-1)
	return -1;

    gmt = gmtime(&t);
    if (gmt == NULL)
	return -1;

    sprintf(string, "%s, %02d %s %4d %02d:%02d:%02d GMT",
	abbreviated_weekdays[gmt->tm_wday],
	gmt->tm_mday,
	abbreviated_months[gmt->tm_mon],
	gmt->tm_year + 1900,
	gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

    return 0;
}


/*
 * Generate a string of particular date.  The string is like as:
 *
 * 	    "Tue, 25 Jun 1997 03:57:33 GMT"
 *
 * This date format is described in RFC 1123.
 * `string' must refer to the buffer with 30 bytes or larger.
 * If succeeds to generate, returns 0.  Otherwise returns -1.
 */
int
http_make_date2(string, date)
    char *string;
    const struct tm *date;
{
    sprintf(string, "%s, %02d %s %4d %02d:%02d:%02d GMT",
	abbreviated_weekdays[date->tm_wday],
	date->tm_mday,
	abbreviated_months[date->tm_mon],
	date->tm_year + 1900,
	date->tm_hour, date->tm_min, date->tm_sec);

    return 0;
}


/*
 * Print date `date' to standard out.
 */
void
http_print_date(date)
    const struct tm *date;
{
    char string[HTTP_DATE_STRING_LENGTH + 1];

    http_make_date2(string, date);
    fputs(string, stdout);
}


/*
 * Compare two dates.
 *
 * If `date1' is later than `date2', returns an integer greater than 0.
 * If `date1' is earlier than `date2', returns an integer less than 0.
 * If `date1' is equal to `date2', returns 0.
 */
int
http_compare_date(date1, date2)
    struct tm *date1;
    struct tm *date2;
{
    /*
     * Not to compare yday(day of year).  `Date1' and/or `date2' may
     * be set by parse_http_date().  The function does not set yday.
     */
    if (date1->tm_year != date2->tm_year)
	return (date1->tm_year - date2->tm_year);

    if (date1->tm_mon != date2->tm_mon)
	return (date1->tm_mon - date2->tm_mon);

    if (date1->tm_mday != date2->tm_mday)
	return (date1->tm_mday - date2->tm_mday);

    if (date1->tm_hour != date2->tm_hour)
	return (date1->tm_hour - date2->tm_hour);

    if (date1->tm_min != date2->tm_min)
	return (date1->tm_min - date2->tm_min);

    return (date1->tm_sec - date2->tm_sec);
}


/*
 * Set orign date to `date'.
 * The origin date means "Mon, 01 Jan 2000 00:00:00 GMT".
 */
void
http_set_origin_date(date)
    struct tm *date;
{
    date->tm_year = 0;
    date->tm_mon  = 0;
    date->tm_mday = 1;
    date->tm_hour = 0;
    date->tm_min  = 0;
    date->tm_sec  = 0;
    date->tm_wday = 1;
}


/*
 * Parse a date string used in HTTP.  Its format must be one of:
 *
 *	Wed, 09 Nov 1994 08:49:37 GMT        ; RFC1123 format
 *	Wednesday, 09-Nov-94 08:49:37 GMT    ; RFC850 format
 *	Wed Nov  9 08:49:37 1994             ; asctime() format.
 *
 * Year, month, day of month, hour, minute, second, and weekday is 
 * stored into `date'.
 *
 * If date is invalid, return -1.  Otherwise return 0.
 */
int
http_parse_date(string, date)
    const char *string;
    struct tm *date;
{
    struct tm temporary_date;

    /*
     * Try RFC1123 format (e.g. "Wed, 09 Nov 1994 08:49:37 GMT").
     */
    if (http_parse_date_internal(&temporary_date, string,
	"%a, %d %b %Y %T GMT") == 0) {
	memcpy(date, &temporary_date, sizeof(struct tm));
	return 0;
    }

    /*
     * Try RFC850 format (e.g. "Wednesday, 09-Nov-94 08:49:37 GMT").
     */
    if (http_parse_date_internal(&temporary_date, string,
	"%A, %d-%b-%y %T GMT") == 0) {
	memcpy(date, &temporary_date, sizeof(struct tm));
	return 0;
    }

    /*
     * Try asctime() format (e.g. "Wed Nov  9 08:49:37 1994").
     */
    if (http_parse_date_internal(&temporary_date, string,
	"%a %b %D %T %Y") == 0) {
	memcpy(date, &temporary_date, sizeof(struct tm));
	return 0;
    }
    return -1;
}


/*
 * Convert a string of 1 digit to an integer.
 */
#define char1_to_int(s) (*(s) - '0')

/*
 * Convert a string of 2 digits to an integer.
 */
#define char2_to_int(s) ((*(s) - '0') * 10 + (*((s) + 1) - '0'))

/*
 * Convert a string of 4 digits to an integer.
 */
#define char4_to_int(s) ((*(s) - '0') * 1000 + (*((s) + 1) - '0') * 100 \
	+ (*((s) + 2) - '0') * 10 + (*((s) + 3) - '0'))

static int
http_parse_date_internal(date, string, format)
    struct tm *date;
    const char *string;
    const char *format;
{
    const char *string_p;
    const char *format_p;
    int i;

    /*
     * Parse `date' according to `format'.
     */
    format_p = format;
    string_p = string;

    while (*format_p != '\0') {
	if (*format_p == ' ') {
	    /*
	     * Space -- Allows one or greater number of spaces or tabs.
	     */
	    if (*string_p != ' ' && *string_p != '\t')
		return -1;
	    format_p++;
	    string_p++;
	    while (*string_p == ' ' || *string_p == '\t')
		string_p++;
	    continue;

	} else if (*format_p == '%') {
	    /*
	     * `%' -- Leads special translation.
	     */
	    switch(*(format_p + 1)) {
	    case 'a':
		/*
		 * '%a' -- Abbreviated weekday name. (Sunday -- Saturday)
		 */
		for (i = 0; i < 7; i++) {
		    if (strncasecmp(abbreviated_weekdays[i], string_p, 3) == 0)
			break;
		}
		if (7 <= i)
		    return -1;
		string_p += 3;
		date->tm_wday = i;
		break;

	    case 'A':
		/*
		 * '%A' -- Full weekday name. (Sunday -- Saturday)
		 */
		for (i = 0; i < 7; i++) {
		    if (strncasecmp(full_weekdays[i], string_p,
			full_weekdays_length[i]) == 0)
			break;
		}
		if (7 <= i)
		    return -1;
		string_p += full_weekdays_length[i];
		date->tm_wday = i;
		break;

	    case 'b':
		/*
		 * '%b' -- Abbreviated month name. (Jan -- Dec)
		 */
		for (i = 0; i < 12; i++) {
		    if (strncasecmp(abbreviated_months[i], string_p, 3) == 0)
			break;
		}
		if (12 <= i)
		    return -1;
		string_p += 3;
		date->tm_mon = i;
		break;

	    case 'd':
		/*
		 * '%d' -- Day of month. (00 -- 31)
		 */
		if (!isdigit(*string_p) || !isdigit(*(string_p + 1)))
		    return -1;
		date->tm_mday = char2_to_int(string_p);
		string_p += 2;
		break;

	    case 'D':
		/*
		 * '%D' -- Day of month.  A space is inserted if less
		 * than 10. ( 0 -- 31)
		 */
		if (!isdigit(*string_p))
		    return -1;
		if (!isdigit(*(string_p + 1))) {
		    date->tm_mday = char1_to_int(string_p);
		    string_p++;
		} else {
		    date->tm_mday = char2_to_int(string_p);
		    string_p += 2;
		}
		break;

	    case 'T':
		/*
		 * '%T' -- time in 24-hour U.S usual format.  HH:MM:SS.
		 * (00:00:00 -- 23:59:59)
		 */
		if (!isdigit(*string_p)
		    || !isdigit(*(string_p + 1))
		    || *(string_p + 2) != ':'
		    || !isdigit(*(string_p + 3))
		    || !isdigit(*(string_p + 4))
		    || *(string_p + 5) != ':'
		    || !isdigit(*(string_p + 6))
		    || !isdigit(*(string_p + 7)))
		    return -1;
		date->tm_hour = char2_to_int(string_p);
		date->tm_min = char2_to_int(string_p + 3);
		date->tm_sec = char2_to_int(string_p + 6);
		string_p += 8;
		break;

	    case 'y':
		/*
		 * '%y' -- Year without century. (00 -- 99)
		 */
		if (!isdigit(*string_p) || !isdigit(*(string_p + 1)))
		    return -1;
		date->tm_year = char2_to_int(string_p);
		if (date->tm_year < 70)
		    date->tm_year += 100;
		string_p += 2;
		break;

	    case 'Y':
		/*
		 * '%Y' -- Year with century. (1900 -- 9999)
		 */
		if (!isdigit(*string_p)
		    || !isdigit(*(string_p + 1))
		    || !isdigit(*(string_p + 2))
		    || !isdigit(*(string_p + 3)))
		    return -1;
		date->tm_year = char4_to_int(string_p) - 1900;
		string_p += 4;
		break;
	    }
	    
	    format_p += 2;

	} else {
	    /*
	     * neither `%' nor ` '.
	     */
	    if (*format_p != *string_p)
		return -1;
	    format_p++;
	    string_p++;
	}
    }

    if (*string_p != '\0')
	return -1;

    return 0;
}


/*
 * Main for test.
 */
#ifdef TEST
int main(argc, argv)
    int argc;
    char *argv[];
{
    struct tm date;

    if (argc < 2) {
	fprintf(stderr, "usage: %s date-string\n", argv[0]);
	exit(1);
    }

    if (http_parse_date(&date, argv[1]) < 0) {
	fprintf(stderr, "bad date string -- \"%s\"\n", argv[1]);
	exit(1);
    }

    printf("year = %d\n", date.tm_year);
    printf("mon = %d\n", date.tm_mon);
    printf("mday = %d\n", date.tm_mday);
    printf("wday = %d\n", date.tm_wday);
    printf("hour = %d\n", date.tm_hour);
    printf("min = %d\n", date.tm_min);
    printf("sec = %d\n", date.tm_sec);
}
#endif

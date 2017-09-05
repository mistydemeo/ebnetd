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
#include <sys/socket.h>
#include <netinet/in.h>

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

#include "httprequest.h"

#ifdef __STDC__
static int http_skip_parameter(const char *, char **);
static int http_skip_token(const char *, char **);
static int http_skip_quoted_string(const char *, char **);
#else
static int http_skip_parameter();
static int http_skip_token();
static int http_skip_quoted_string();
#endif

/*
 * Delete white spaces at the tail and beginning of the HTTP header.
 */
void
http_remove_header_spaces(header)
    char *header;
{
    char *non_space_start;
    char *non_space_end;

    /*
     * Delete white spaces at the tail of the string.
     */
    non_space_end = header + strlen(header) - 1;
    while (header <= non_space_end
	&& (*non_space_end == ' ' || *non_space_end == '\t'))
	*non_space_end-- = '\0';
    non_space_end++;

    /*
     * Delete white spaces at the beginning of the string.
     */
    non_space_start = header;
    while (*non_space_start == ' ' || *non_space_start == '\t')
	non_space_start++;
    memmove(header, non_space_start, non_space_end - non_space_start);
    *(header + (non_space_end - non_space_start)) = '\0';
}

/*
 * Parse an HTTP method name, such as "GET", "HEAD" and so on.
 *
 * This function parses method name refered by `method_string'.
 * If the HTTP name is valid, the function puts the HTTP method code
 * corresponding to the name into `method_code', and returns 0.
 * Otherwise, it puts HTTP_METHOD_UNKNOWN and returns -1.
 */
int
http_parse_method(method_string, method_code)
    const char *method_string;
    HTTP_Method_Code *method_code;
{
    if (strcmp(method_string, "OPTIONS") == 0)
	*method_code = HTTP_METHOD_OPTIONS;
    else if (strcmp(method_string, "GET") == 0)
	*method_code = HTTP_METHOD_GET;
    else if (strcmp(method_string, "HEAD") == 0)
	*method_code = HTTP_METHOD_HEAD;
    else if (strcmp(method_string, "POST") == 0)
	*method_code = HTTP_METHOD_POST;
    else if (strcmp(method_string, "PUT") == 0)
	*method_code = HTTP_METHOD_PUT;
    else if (strcmp(method_string, "DELETE") == 0)
	*method_code = HTTP_METHOD_DELETE;
    else if (strcmp(method_string, "TRACE") == 0)
	*method_code = HTTP_METHOD_TRACE;
    else {
	*method_code = HTTP_METHOD_UNKNOWN;
	return -1;
    }

    return 0;
}


/*
 * Parse `Connection' header value.
 *
 * This function parses `connection_string' which records value part
 * of the HTTP header `Connection' such as "close".
 * 
 *     Connection        = "Connection" ":" 1#(connection-token)
 *     connection-token  = token
 *
 * If the value is valid, the function puts the HTTP connection codes
 * corresponding to the value into `connection_codes', and returns 0.
 * Otherwise returns -1.
 */
int
http_parse_connection(connection_string, connection_codes)
    const char *connection_string;
    HTTP_Connection_Code *connection_codes;
{
    const char *string_p = connection_string;
    const char *end_p;

    for (;;) {
	if (http_skip_token(string_p, (char **)&end_p) < 0)
	    return -1;
	if (strncasecmp("close", string_p, end_p - string_p) == 0)
	    connection_codes[HTTP_CONNECTION_CLOSE] = 1;
	string_p = end_p;
	while (*string_p == ' ' || *string_p == '\t')
	    string_p++;
	if (*string_p == '\0')
	    break;
	else if (*string_p != ',')
	    return -1;
	string_p++;
	while (*string_p == ' ' || *string_p == '\t')
	    string_p++;
    }

    return 0;
}


/*
 * Parse `Transfer-Coding' header value.
 *
 * This function parses `transfer_string' which records value part of
 * the HTTP header `Transfer-Encoding', such as "chunked".
 *
 *     Transfer-Encoding       = "Transfer-Encoding" ":" 1#transfer-coding
 *     transfer-coding         = "chunked" | transfer-extension
 *     transfer-extension      = token *( ";" parameter )
 * 
 * If the value is valid, the function puts the HTTP tarnsfer coding
 * codes corresponding to the value into `coding_codes', and returns 0.
 * Otherwise, returns -1.
 */
int
http_parse_transfer_coding(transfer_string, transfer_codes)
    const char *transfer_string;
    HTTP_Transfer_Coding_Code *transfer_codes;
{
    const char *string_p = transfer_string;
    const char *end_p;

    for (;;) {
	if (http_skip_token(string_p, (char **)&end_p) < 0)
	    return -1;
	if (strncasecmp("chunked", string_p, end_p - string_p) == 0)
	    transfer_codes[HTTP_TRANSFER_CODING_CHUNKED] = 1;
	string_p = end_p;
	while (*string_p == ' ' || *string_p == '\t')
	    string_p++;
	if (*string_p == ';'
	    && http_skip_parameter(string_p + 1, (char **)&string_p) < 0)
	    return -1;
	while (*string_p == ' ' || *string_p == '\t')
	    string_p++;
	if (*string_p == '\0')
	    break;
	else if (*string_p != ',')
	    return -1;
	string_p++;
	while (*string_p == ' ' || *string_p == '\t')
	    string_p++;
    }

    return 0;
}


/*
 * Parse `Content-Length' header value.
 *
 * This function parses `length_string' which records value part of
 * the HTTP header `Content-Length'.
 * 
 * If the value is valid, the function puts the length into `length',
 * and returns 0.  Otherwise, it puts HTTP_TRANSFER_CODING_UNKNOWN
 * and returns -1.
 */
int
http_parse_content_length(length_string, length)
    const char *length_string;
    size_t *length;
{
    const char *string_p = length_string;

    *length = 0;
    while (isdigit(*string_p)) {
	*length = *length * 10 + (*string_p - '0');
	string_p++;
    }
    if (*string_p != '\0')
	return -1;

    return 0;
}


/*
 * Parse `Host' header value.
 *
 * This function parses `host_and_port' which records value part of the
 * HTTP header `Host-Coding'.  The header valude must be host name
 * (e.g. "www.w3.org") or IP address (e.g. "192.168.20.1"), with optional
 * port number (e.g. "www.w3.org:80", "192.168.20.1:80").
 *
 * Upon return, the given host name or IP address is copied to `host'
 * and port number is set to `*port'.  `host' must refers to a buffer
 * with (HTTP_MAX_HOST_NAME_LENGTH + 1) bytes.
 * 
 * If the header field is valid, 0 is returned.  Otherwise, -1 is
 * returned.
 */
int
http_parse_host(host_and_port, host, port)
    const char *host_and_port;
    char *host;
    in_port_t *port;
{
    size_t host_length;
    const char *colon;
    const char *port_p;

    /*
     * We initially set `port' to 80, and `host' to empty string.
     */
    *port = 80;
    *host = '\0';

    colon = strchr(host_and_port, ':');
    if (colon == NULL) {
	/*
	 * There is no colon (`:') in `host_and_port'.
	 * The header field has host name only.
	 */
	host_length = strlen(host_and_port);

    } else {
	/*
	 * There is a colon (`:') in `host_and_port'.
	 * The header field has both host name and port number.
	 */
	host_length = colon - host_and_port;

	port_p = colon + 1;
	*port = 0;
	while (isdigit(*port_p)) {
	    *port = *port * 10 + (*port_p - '0');
	    port_p++;
	}
	if (*port_p != '\0')
	    return -1;
    }

    /*
     * Copy the host name to `host'.
     */
    if (host_length == 0 || HTTP_MAX_HOST_NAME_LENGTH < host_length)
	return -1;
    memcpy(host, host_and_port, host_length);
    *(host + host_length) = '\0';

    return 0;
}


/*
 * Parse an HTTP version string.
 *
 * The string must begin with "HTTP/", and followed by a mojor
 * and minor versions separated by a comma (e.g. "HTTP/1.0").
 *
 * This function returns 0 if the line is a valid string, and
 * returns -1 otherwise.
 */
int
http_parse_version(version_string, version)
    const char *version_string;
    HTTP_Version_Code *version;
{
    const char *string_p = version_string;
    int major_version = 0;
    int minor_version = 9;

    /*
     * Version string must be start with "HTTP/".
     */
    if (strncasecmp(version_string, "HTTP/", 5) != 0)
	return -1;

    /*
     * Take one or more digits, as the major_version version_string.
     */
    string_p = version_string + 5;
    if (!isdigit(*string_p))
	return -1;
    major_version = *string_p - '0';
    string_p++;
    while (isdigit(*string_p)) {
	major_version = major_version * 10 + (*string_p - '0');
	string_p++;
    }

    /*
     * Major version_string must be followed by `.'.
     */
    if (*string_p != '.')
	return -1;
    string_p++;

    /*
     * Take one or more digits, as the minor_version version_string.
     */
    if (!isdigit(*string_p))
	return -1;
    minor_version = *string_p - '0';
    string_p++;
    while (isdigit(*string_p)) {
	minor_version = minor_version * 10 + (*string_p - '0');
	string_p++;
    }

    /*
     * No character is permitted after the minor_version version_string.
     */
    if (*string_p != '\0')
	return -1;

    /*
     * Set `version'.
     */
    if (major_version == 1 && minor_version == 0)
	*version = HTTP_VERSION_1_0;
    else if (major_version == 1 && minor_version == 1)
	*version = HTTP_VERSION_1_1;
    else if (major_version == 1 && 2 <= minor_version)
	*version = HTTP_VERSION_HIGH;
    else if (2 <= major_version)
	*version = HTTP_VERSION_HIGH;
    else
	return -1;

    return 0;
}


/*
 * Skip a parameter.
 *
 *     parameter      = attribute "=" value
 *     attribute      = token
 *     value          = token | quoted-string
 *
 * Return 0 upon success, -1 otherwise.  If `end_p' is non-NULL,
 * the position to next character of end of the token is stored in
 * `*end_p'.
 */
static int
http_skip_parameter(string, end_p)
    const char *string;
    char **end_p;
{
    const char *string_p = string;

    /*
     * Skip a token and whitespaces around it.
     */
    while (*string_p == ' ' || *string_p == '\t')
	string_p++;
    if (http_skip_token(string_p, (char **)&string_p) < 0)
	return -1;
    while (*string_p == ' ' || *string_p == '\t')
	string_p++;
    
    /*
     * Skip "=".
     */
    if (*string_p != '=')
	return -1;
    string_p++;

    /*
     * Skip a token or quoted-string, and whitespaces in front of it.
     */
    while (*string_p == ' ' || *string_p == '\t')
	string_p++;

    if (*string_p == '\"') {
	if (http_skip_quoted_string(string_p, (char **)&string_p) < 0)
	    return -1;
    } else {
	if (http_skip_token(string_p, (char **)&string_p) < 0)
	    return -1;
    }
    *end_p = (char *)string_p;

    return 0;
}


/*
 * Table to test a given character is CTL or separator.
 *
 *     CTL            = <any US-ASCII control character
 *                      (octets 0 - 31) and DEL (127)>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const char ctl_separators_table[] = {
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x00 ... 0x0f    */
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x10 ... 0x1f    */
    1, 0, 1, 0,  0, 0, 0, 0,  1, 1, 0, 0,  1, 0, 0, 1, /*  !"#$%&'()*+,-./ */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 1,  1, 1, 1, 1, /* 0123456789:;<=>? */
    1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0, /* PQRSTUVWXYZ[\]^_ */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* `abcdefghijklmno */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  0, 1, 0, 1, /* pqrstuvwxyz{|}~  */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x80 ... 0x8f    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x90 ... 0x9f    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xa0 ... 0xaf    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xb0 ... 0xbf    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xc0 ... 0xcf    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xd0 ... 0xdf    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xe0 ... 0xef    */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0xf0 ... 0xff    */
};    

/*
 * Skip a token beginning at `string'.
 *
 *     token          = 1*<any CHAR except CTLs or separators>
 *     CTL            = <any US-ASCII control character
 *                      (octets 0 - 31) and DEL (127)>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 *
 * Return 0 upon success, -1 otherwise.  If `end_p' is non-NULL,
 * the position to next character of end of the token is stored in
 * `*end_p'.
 */

static int
http_skip_token(string, end_p)
    const char *string;
    char **end_p;
{
    const unsigned char *string_p = (const unsigned char *)string;

    while (*string_p != '\0') {
	if (ctl_separators_table[*string_p])
	    break;
	string_p++;
    }

    if (string_p == (const unsigned char *)string)
	return -1;
    if (end_p != NULL)
	*end_p = (char *)string_p;

    return 0;
}


/*
 * Skip a quoted string beginning at `string'.
 *
 *     quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
 *     qdtext         = <any TEXT except <">>
 *     quoted-pair    = "\" CHAR
 *
 * Return 0 upon success, -1 otherwise.  If `end_p' is non-NULL,
 * the position to next character of end of the token is stored in
 * `*end_p'.
 */
static int
http_skip_quoted_string(string, end_p)
    const char *string;
    char **end_p;
{
    const char *p = string;

    if (*p != '\"')
	return -1;
    p++;
    for (;;) {
	if (*p == '\0') {
	    return -1;
	} else if (*p == '\"') {
	    p++;
	    break;
	} else if (*p == '\\') {
	    p++;
	    if (*p == '\0')
		return -1;
	}
	p++;
    }

    if (end_p != NULL)
	*end_p = (char *)p;

    return 0;
}


/*
 * Return a method name corresponding with `method_code'.
 * This function does the reverse of http_parse_method().
 */
const char *
http_method_string(method_code)
    HTTP_Method_Code method_code;
{
    switch (method_code) {
    case HTTP_METHOD_OPTIONS:
	return "OPTIONS";
    case HTTP_METHOD_GET:
	return "GET";
    case HTTP_METHOD_HEAD:
	return "HEAD";
    case HTTP_METHOD_POST:
	return "POST";
    case HTTP_METHOD_PUT:
	return "PUT";
    case HTTP_METHOD_DELETE:
	return "DELETE";
    case HTTP_METHOD_TRACE:
	return "TRACE";
    }

    return "unknown";
}


/*
 * Return a HTTP version string corresponding with `version_code'.
 */
const char *
http_version_string(version_code)
    HTTP_Version_Code version_code;
{
    switch (version_code) {
    case HTTP_VERSION_0_9:
	return "0.9";
    case HTTP_VERSION_1_0:
	return "1.0";
    case HTTP_VERSION_1_1:
	return "1.1";
    case HTTP_VERSION_HIGH:
	return "greater-than-1.1";
    }

    return "unknown";
}



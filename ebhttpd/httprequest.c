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
#include <syslog.h>

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

#ifdef HAVE_SYS_PARAMS_H
#include <sys/params.h>
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

#include "fakelog.h"
#include "linebuf.h"
#include "httprequest.h"
#include "httpdate.h"

/*
 * Length of short buffer.
 */
#define SHORT_BUFFER_LENGTH 		15

/*
 * Unexported functions.
 */
#ifdef __STDC__
static void http_parse_header(HTTP_Request *, char *);
static int http_receive_request_line(HTTP_Request *, Line_Buffer *);
static int http_receive_request_headers(HTTP_Request *, Line_Buffer *);
static ssize_t http_receive_request_entity(HTTP_Request *, Line_Buffer *);
static ssize_t http_receive_request_chunked_entity(HTTP_Request *,
    Line_Buffer *);
#else
static void http_parse_header();
static int http_receive_request_line();
static int http_receive_request_headers();
static ssize_t http_receive_request_entity();
static ssize_t http_receive_request_chunked_entity();
#endif

/*
 * Receive an HTTP request reading from `line_buffer'.
 *
 * The request generally consists of request line, header fields and
 * optinal entity.  Information of the given request is set to `request'.
 */
int
http_receive_request(request, line_buffer)
    HTTP_Request *request;
    Line_Buffer *line_buffer;
{
    if (http_receive_request_line(request, line_buffer) < 0)
	return -1;

    if (HTTP_VERSION_1_0 <= request->version) {
	if (http_receive_request_headers(request, line_buffer) < 0)
	    return -1;
    }

    /*
     * If the request is HTTP/1.0 or HTTP/0.9, or the request has the
     * "Connection: close" header field, the connection is closed after
     * responding.
     */
    if (request->version <= HTTP_VERSION_1_0
	|| request->connections[HTTP_CONNECTION_CLOSE])
	request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;

    /*
     * HTTP/1.1 and higher reuqire Host header field.
     */
    if (HTTP_VERSION_1_1 <= request->version) {
	if (request->host[0] == '\0') {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	}
    }

    /*
     * Receive entity.
     */
    if (request->transfer_codings[HTTP_TRANSFER_CODING_CHUNKED]) {
	if (http_receive_request_chunked_entity(request, line_buffer) < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
    } else if (0 < request->content_length) {
	if (http_receive_request_entity(request, line_buffer) < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
    }

    return 0;
}


/*
 * Parse an received header.
 */
static void
http_parse_header(request, header)
    HTTP_Request *request;
    char *header;
{
    char *header_name;
    char *header_value;
    char *colon;

    /*
     * Split the line into header name and its value.
     */
    colon = strchr(header, ':');
    if (colon == NULL)
	return;
    *colon = '\0';

    /*
     * Delete white spaces at the beginning and tail of the name
     * and value.
     */
    header_name = header;
    http_remove_header_spaces(header_name);
    header_value = colon + 1;
    http_remove_header_spaces(header_value);

    /*
     * Parse the HTTP header.
     */
    if (strcasecmp(header_name, "Connection") == 0) {
	http_parse_connection(header_value, request->connections);

    } else if (strcasecmp(header_name, "Host") == 0) {
	if (http_parse_host(header_value, request->host, &request->port) < 0) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	}
    } else if (strcasecmp(header_name, "Transfer-Encoding") == 0) {
	if (http_parse_transfer_coding(header_value,
	    request->transfer_codings) < 0) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	}
    } else if (strcasecmp(header_name, "Content-Length") == 0) {
	if (http_parse_content_length(header_value,
	    &request->content_length) < 0) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	}
    } else if (strcasecmp(header_name, "If-Modified-Since") == 0) {
	if (http_parse_date(header_value, &request->if_modified_since) < 0) {
	    http_set_origin_date(&request->if_modified_since);
	}
    }
}


/*
 * Initialize `request'.
 */
void
http_initialize_request(request)
    HTTP_Request *request;
{
    int i;

    request->method = HTTP_METHOD_UNKNOWN;
    url_parts_initialize(&request->url);
    request->version = HTTP_VERSION_0_9;

    request->content_length = 0;
    request->host[0] = '\0';
    request->port = 80;
    for (i = 0; i < HTTP_MAX_CONNECTIONS; i++)
	request->connections[i] = 0;
    for (i = 0; i < HTTP_MAX_TRANSFER_CODINGS; i++)
	request->transfer_codings[i] = 0;
    http_set_origin_date(&request->if_modified_since);

    request->response_code = HTTP_RESPONSE_OK;
    request->status = HTTP_STATUS_OPEN;
}


/*
 * Finalize `request'.
 */
void 
http_finalize_request(request)
    HTTP_Request *request;
{
    url_parts_finalize(&request->url);
}


/*
 * Receive an HTTP request line reading from `line_buffer'.
 * Here is an example of request line.
 *
 *     GET /some/where/index.html HTTP/1.1
 *
 * It sets `method', `url', `version', of `request' struct members,
 * and also sets `response' and `status' if an error occurs.
 *
 * This function returns -1 upon critical error that `status' is set
 * to HTTP_STATUS_CLOSE_IMMEDIATELY.  Otherwise, it returns 0.
 */
static int
http_receive_request_line(request, line_buffer)
    HTTP_Request *request;
    Line_Buffer *line_buffer;
{
    char line[HTTP_MAX_LINE_LENGTH + 1];
    const char *line_p;
    ssize_t line_length;
    char method_string[HTTP_MAX_METHOD_LENGTH + 1];
    char *method_p;
    size_t method_length;
    char uri_string[HTTP_MAX_URI_LENGTH + 1];
    char *uri_p;
    size_t uri_length;
    char version_string[HTTP_MAX_VERSION_LENGTH + 1];
    char *version_p;
    size_t version_length;

    /*
     * Read a request line.
     *
     * We give up parsing the request, when the received line is too long.
     * In this case, we suppose it is HTTP/1.0 request and method name is
     * unknown.  This is negligence.
     */
    for (;;) {
	line_length = read_line_buffer(line_buffer, line,
	    HTTP_MAX_LINE_LENGTH);
	if (line_length < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	syslog(LOG_DEBUG, "debug: received line %s", line);

	if (HTTP_MAX_LINE_LENGTH < line_length) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	    request->method = HTTP_METHOD_UNKNOWN;
	    *(line + SHORT_BUFFER_LENGTH) = '\0';	    
	    syslog(LOG_INFO, "too long request line: %s", line);
	    if (skip_line_buffer(line_buffer) < 0) {
		request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
		return -1;
	    }
	    return 0;
	}
	line_p = line;

	/*
	 * Skip successive white spaces.
	 * Ignore the line if there is non-white-space in it.
	 */
	while (*line_p == ' ' || *line_p == '\t')
	    line_p++;
	if (*line_p != '\0')
	    break;
    }

    /*
     * Get method.
     */
    method_length = 0;
    method_p = method_string;
    while (*line_p != ' ' && *line_p != '\t' && *line_p != '\0') {
	method_length++;
	if (HTTP_MAX_METHOD_LENGTH < method_length) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	    *method_p = '\0';
	    syslog(LOG_INFO, "too long method name: %s", method_string);
	    return 0;
	}
	*method_p++ = *line_p++;
    }
    *method_p = '\0';

    /*
     * Parse method name.
     */
    if (http_parse_method(method_string, &request->method) < 0)
	request->response_code = HTTP_RESPONSE_NOT_IMPLEMENTED;

    /*
     * Skip successive white spaces behind the method name.
     */
    while (*line_p == ' ' || *line_p == '\t')
	line_p++;
    if (*line_p == '\0') {
	request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	*method_p = '\0';
	syslog(LOG_INFO, "request missing URI: %s", line);
	return 0;
    }

    /*
     * Get URI.
     */
    uri_length = 0;
    uri_p = uri_string;
    while (*line_p != ' ' && *line_p != '\t' && *line_p != '\0') {
	uri_length++;
	if (HTTP_MAX_URI_LENGTH < uri_length) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	    *uri_p = '\0';
	    syslog(LOG_INFO, "too long URI: %s", uri_string);
	    return 0;
	}
	*uri_p++ = *line_p++;
    }
    *uri_p = '\0';

    /*
     * Parse URI as URL.
     */
    if (url_parts_parse(&request->url, uri_string) < 0) {
	request->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	syslog(LOG_INFO, "memory exhausted");
	return 0;
    }

    /*
     * Skip successive white spaces behind the URI.
     * If reachs to the end of line, HTTP/0.9 simple request is assumed.
     */
    while (*line_p == ' ' || *line_p == '\t')
	line_p++;
    if (*line_p == '\0') {
	request->version = HTTP_VERSION_0_9;
	return 0;
    }

    /*
     * Get HTTP version.
     */
    version_length = 0;
    version_p = version_string;
    while (*line_p != ' ' && *line_p != '\t' && *line_p != '\0') {
	version_length++;
	if (HTTP_MAX_VERSION_LENGTH < version_length) {
	    request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	    request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	    *version_string = '\0';
	    syslog(LOG_INFO, "too long HTTP version: %s", version_string);
	    return 0;
	}
	*version_p++ = *line_p++;
    }
    *version_p = '\0';

    /*
     * Parse HTTP version.
     */
    if (http_parse_version(version_string, &request->version) < 0) {
	request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	syslog(LOG_INFO, "invalid HTTP version: %s", version_string);
	return 0;
    }

    /*
     * Skip successive white spaces behind the HTTP version.
     * It is bad request if an extra argument is found.
     */
    while (*line_p == ' ' || *line_p == '\t')
	line_p++;
    if (*line_p != '\0') {
	request->response_code = HTTP_RESPONSE_BAD_REQUEST;
	request->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	syslog(LOG_INFO, "request has an extra argument: %s", line_p);
	return 0;
    }

    return 0;
}


/*
 * Receive HTTP request header fields reading from `line_buffer'.
 * Here is an example of request header fields.
 *
 *     Host: www.w3.org
 *     Connection: close
 *     User-Agent: Internet Exploder 1.0
 *
 * It sets header related members in `request', and also sets `response'
 * and `status' if an error occurs.
 *
 * This function returns -1 upon critical error that `status' is set
 * to HTTP_STATUS_CLOSE_IMMEDIATELY.  Otherwise, it returns 0.
 */
static int
http_receive_request_headers(request, line_buffer)
    HTTP_Request *request;
    Line_Buffer *line_buffer;
{
    size_t header_length;
    ssize_t line_length;
    char line[HTTP_MAX_LINE_LENGTH + 1];
    char header[HTTP_MAX_HEADER_LENGTH + 1];

    /*
     * Read header fields.
     */
    header_length = 0;
    for (;;) {
	/*
	 * Read a next line.
	 */
	line_length = read_line_buffer(line_buffer, line,
	    HTTP_MAX_LINE_LENGTH);
	if (line_length < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	syslog(LOG_DEBUG, "debug: received line %s", line);

	/*
	 * If the first character of the received line is not a whitespace,
	 * it terminates a privous header field.  Parse the header.
	 */
	if (*line != ' ' && *line != '\t' && 0 < header_length) {
	    http_parse_header(request, header);
	}

	/*
	 * An empty line means the end of header fields.
	 */
	if (line_length == 0)
	    break;

	if (HTTP_MAX_LINE_LENGTH < line_length) {
	    /*
	     * The received line is too long.
	     * An unterminted header field is discarded if having.
	     */
	    if (skip_line_buffer(line_buffer) < 0) {
		request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
		return -1;
	    }
	    *header = '\0';
	    header_length = 0;

	} else if (*line == ' ' || *line == '\t') {
	    /*
	     * Since the line with a whitespace as the first character
	     * is a continuous line, we add it to the previous header
	     * line unless the total length of the header doesn't exceeds
	     * HTTP_MAX_HEADER_LENGTH.
	     */
	    if (header_length == 0) {
		; /* nothing to do */
	    } else if (HTTP_MAX_HEADER_LENGTH
		< header_length + line_length + 1) {
		*header = '\0';
		header_length = 0;
	    } else {
		memcpy(header + header_length, line, line_length);
		*(header + header_length + line_length) = ' ';
		*(header + header_length + line_length + 1) = '\0';
		header_length += line_length + 1;
	    }
	} else {
	    /*
	     * The first character of the line is not a whitespace.
	     * It creates a new header field.  Copy the line to `header'.
	     */
	    memcpy(header, line, line_length);
	    *(header + line_length) = ' ';
	    *(header + line_length + 1) = '\0';
	    header_length = line_length + 1;
	}
    }

    return 0;
}

/*
 * Read request entity.
 */
static ssize_t
http_receive_request_entity(request, line_buffer)
    HTTP_Request *request;
    Line_Buffer *line_buffer;
{
    char entity_buffer[HTTP_MAX_LINE_LENGTH + 1];
    size_t rest_length = request->content_length;
    size_t read_length;

    while (0 < rest_length) {
	if (HTTP_MAX_LINE_LENGTH + 1 < rest_length)
	    read_length = HTTP_MAX_LINE_LENGTH + 1;
	else
	    read_length = rest_length;
	if (binary_read_line_buffer(line_buffer, entity_buffer, read_length)
	    < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	rest_length -= read_length;
    }

    syslog(LOG_DEBUG, "debug: received entity (%d bytes)",
	request->content_length);

    return request->content_length;
}


/*
 * Read request entity with chunked transder encoding.
 */
static ssize_t
http_receive_request_chunked_entity(request, line_buffer)
    HTTP_Request *request;
    Line_Buffer *line_buffer;
{
    ssize_t line_length;
    size_t chunk_length;
    size_t chunk_rest_length;
    ssize_t chunk_read_length;
    char *end_p;
    char line[HTTP_MAX_LINE_LENGTH + 1];
    char *line_p;

    request->content_length = 0;

    /*
     * Read chunks.
     */
    for (;;) {
	/*
	 * Read a line which contains chunk size and chunk-extension.
	 */
	line_length = read_line_buffer(line_buffer, line,
	    HTTP_MAX_LINE_LENGTH);
	if (line_length < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	line_p = line;
	while (*line_p == ' ' || *line_p == '\t')
	    line_p++;

	if (!isdigit(*line_p)) {
	    syslog(LOG_INFO, "invalid chunked entity transfer");
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	chunk_length = (size_t) strtol(line, &end_p, 16);
	while (*end_p == ' ' || *end_p == '\t')
	    end_p++;
	if (*end_p != ';' && *end_p != '\0')
	    return -1;
	if (chunk_length == 0)
	    break;

	/*
	 * Read chunk-data.
	 */
	chunk_rest_length = chunk_length;
	while (0 < chunk_rest_length) {
	    if (HTTP_MAX_LINE_LENGTH + 1 < chunk_rest_length)
		chunk_read_length = HTTP_MAX_LINE_LENGTH + 1;
	    else
		chunk_read_length = chunk_rest_length;
	    if (binary_read_line_buffer(line_buffer, line,
		chunk_read_length) < 0) {
		request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
		return -1;
	    }
	    chunk_rest_length -= chunk_read_length;
	}

	request->content_length += chunk_length;
    }

    syslog(LOG_DEBUG, "debug: received entity (%d bytes)",
	request->content_length);

    /*
     * Read a trailer.
     */
    for (;;) {
	/*
	 * Read a next line.
	 */
	line_length = read_line_buffer(line_buffer, line,
	    HTTP_MAX_LINE_LENGTH);
	if (line_length < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
	syslog(LOG_DEBUG, "debug: received chunked trailer %s", line);

	/*
	 * An empty line means the end of the trailer.
	 */
	if (line_length == 0)
	    break;

	/*
	 * If the received line is too long, skip the line.
	 */
	if (HTTP_MAX_LINE_LENGTH < line_length
	    && skip_line_buffer(line_buffer) < 0) {
	    request->status = HTTP_STATUS_CLOSE_IMMEDIATELY;
	    return -1;
	}
    }

    return request->content_length;
}



/*
 * Functions to access members in `request'.
 */
HTTP_Method_Code
http_request_method(request)
    HTTP_Request *request;
{
    return request->method;
}

const char *
http_request_method_string(request)
    HTTP_Request *request;
{
    return http_method_string(request->method);
}

const char *
http_request_url(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_url(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_scheme(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_scheme(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_user(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_user(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_password(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_password(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_host(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_host(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_port(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_port(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_path(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_path(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_params(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_params(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_query(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_query(&request->url);
    return (part != NULL) ? part : "";
}

const char *
http_request_url_fragment(request)
    HTTP_Request *request;
{
    const char *part;

    part = url_parts_fragment(&request->url);
    return (part != NULL) ? part : "";
}

HTTP_Version_Code
http_request_version(request)
    HTTP_Request *request;
{
    return request->version;
}

const char *
http_request_version_string(request)
    HTTP_Request *request;
{
    return http_version_string(request->version);
}

size_t
http_request_content_length(request)
    HTTP_Request *request;
{
    return request->content_length;
}

const char *
http_request_host(request)
    HTTP_Request *request;
{
    return request->host;
}

int
http_request_port(request)
    HTTP_Request *request;
{
    return request->port;
}

struct tm *
http_request_if_modified_since(request)
    HTTP_Request *request;
{
    return &(request->if_modified_since);
}

HTTP_Response_Code
http_request_response_code(request)
    HTTP_Request *request;
{
    return request->response_code;
}

HTTP_Status_Code
http_request_status(request)
    HTTP_Request *request;
{
    return request->status;
}

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

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

#include "urlparts.h"
#include "httprescode.h"
#include "linebuf.h"

/*
 * Type for HTTP method code.
 */
typedef int HTTP_Method_Code;
typedef int HTTP_Version_Code;

/*
 * HTTP method codes.
 */
#define HTTP_METHOD_OPTIONS	0
#define HTTP_METHOD_GET		1
#define HTTP_METHOD_HEAD	2
#define HTTP_METHOD_POST	3
#define HTTP_METHOD_PUT		4
#define HTTP_METHOD_DELETE	5
#define HTTP_METHOD_TRACE	6

#define HTTP_METHOD_UNKNOWN	-1

/*
 * HTTP version codes.
 */
#define HTTP_VERSION_0_9	0
#define HTTP_VERSION_1_0	1
#define HTTP_VERSION_1_1	2
#define HTTP_VERSION_HIGH	3

/*
 * HTTP trasfer coding type, and its values.
 */
typedef int HTTP_Transfer_Coding_Code;

#define HTTP_MAX_TRANSFER_CODINGS	1
#define HTTP_TRANSFER_CODING_CHUNKED	0

/*
 * HTTP connection type, and its values.
 */
typedef int HTTP_Connection_Code;

#define HTTP_MAX_CONNECTIONS		1
#define HTTP_CONNECTION_CLOSE		0

/*
 * HTTP internal status 
 */
typedef int HTTP_Status_Code;

#define HTTP_STATUS_CLOSE_IMMEDIATELY	0
#define HTTP_STATUS_CLOSE_AFTER_RESPOND	1
#define HTTP_STATUS_OPEN		2

/*
 * Maximum length definitions.
 */
#define HTTP_MAX_HOST_NAME_LENGTH	63
#define HTTP_MAX_LINE_LENGTH		1023
#define HTTP_MAX_HEADER_LENGTH		511
#define HTTP_MAX_URI_LENGTH		1023
#define HTTP_MAX_METHOD_LENGTH		15
#define HTTP_MAX_VERSION_LENGTH		15

/*
 * HTTP request.
 */
typedef struct {
    /*
     * Method (e.g. GET, POST, ...).
     */
    HTTP_Method_Code method;

    /*
     * URL (e.g. "http://www.w3.org/").
     */
    URL_Parts url;

    /*
     * HTTP major and minor versions.
     */
    HTTP_Version_Code version;

    /*
     * Content length specified by `Content-Length' 
     */
    size_t content_length;

    /*
     * Host name or IP address, and port specified by the `Host'
     * header field.
     */
    char host[HTTP_MAX_HOST_NAME_LENGTH + 1];
    in_port_t port;

    /*
     * Values of `Transfer-Encoding' header field.
     */
    HTTP_Transfer_Coding_Code transfer_codings[HTTP_MAX_TRANSFER_CODINGS];

    /*
     * Values of `Connection' header field.
     */
    HTTP_Connection_Code connections[HTTP_MAX_CONNECTIONS];

    /*
     * Date specified by `If-Modified-Since'.
     */
    struct tm if_modified_since;

    /*
     * Response code.
     * (If the request parser finds an error, the parser sets this).
     */
    HTTP_Response_Code response_code;

    /*
     * Parser status.
     */
    HTTP_Status_Code status;

} HTTP_Request;

/*
 * Function Declarations.
 */
#ifdef __STDC__
/* httprequest.c */
void http_initialize_request(HTTP_Request *);
void http_finalize_request(HTTP_Request *);
int http_receive_request(HTTP_Request *, Line_Buffer *);

HTTP_Method_Code http_request_method(HTTP_Request *);
const char *http_request_method_string(HTTP_Request *);
const char *http_request_url(HTTP_Request *);
const char *http_request_url_scheme(HTTP_Request *);
const char *http_request_url_user(HTTP_Request *);
const char *http_request_url_password(HTTP_Request *);
const char *http_request_url_host(HTTP_Request *);
const char *http_request_url_port(HTTP_Request *);
const char *http_request_url_path(HTTP_Request *);
const char *http_request_url_params(HTTP_Request *);
const char *http_request_url_query(HTTP_Request *);
const char *http_request_url_fragment(HTTP_Request *);
HTTP_Version_Code http_request_version(HTTP_Request *);
const char *http_request_version_string(HTTP_Request *);
size_t http_request_content_length(HTTP_Request *);
const char *http_request_host(HTTP_Request *);
int http_request_port(HTTP_Request *);
struct tm *http_request_if_modified_since(HTTP_Request *);
HTTP_Response_Code http_request_response_code(HTTP_Request *);
HTTP_Status_Code http_request_status(HTTP_Request *);

/* httpmisc.c */
void http_remove_header_spaces(char *);
int http_parse_method(const char *, HTTP_Method_Code *);
int http_parse_connection(const char *, HTTP_Connection_Code *);
int http_parse_transfer_coding(const char *, HTTP_Transfer_Coding_Code *);
int http_parse_content_length(const char *, size_t *);
int http_parse_host(const char *, char *, in_port_t *);
int http_parse_version(const char *, HTTP_Version_Code *);
const char *http_method_string(HTTP_Method_Code);
const char *http_version_string(HTTP_Version_Code);

#else /* not __STDC__ */

/* httprequest.c */
void http_initialize_request();
void http_finalize_request();
int http_receive_request();

HTTP_Method_Code http_request_method();
const char *http_request_method_string();
const char *http_request_url();
const char *http_request_url_scheme();
const char *http_request_url_user();
const char *http_request_url_password();
const char *http_request_url_host();
const char *http_request_url_port();
const char *http_request_url_path();
const char *http_request_url_params();
const char *http_request_url_query();
const char *http_request_url_fragment();
HTTP_Version_Code http_request_version();
const char *http_request_version_string();
size_t http_request_content_length();
const char *http_request_host();
int http_request_port();
struct tm *http_request_if_modified_since();
HTTP_Response_Code http_request_response_code();
HTTP_Status_Code http_request_status();

/* httpmisc.c */
void http_remove_header_spaces();
int http_parse_method();
int http_parse_connection();
int http_parse_transfer_coding();
int http_parse_content_length();
int http_parse_host();
int http_parse_version();
const char *http_method_string();
const char *http_version_string();

#endif /* not __STDC__ */

#endif /* not HTTPREQUEST_H */

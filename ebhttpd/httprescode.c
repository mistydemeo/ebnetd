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

#include "httprescode.h"

/*
 * HTTP response codes.
 */
/* 1xx -- Informational */
#define RESPONSE_CODE_COUNT_1XX		2
static const char *response_code_messages_1xx[] = {
    "Continue",
    "Switching Protocols"
};

/* 2xx -- Successful */
#define RESPONSE_CODE_COUNT_2XX		7
static const char *response_code_messages_2xx[] = {
    "Ok",
    "Created",
    "Accepted",
    "Non Authoritative Information",
    "No Content",
    "Reset Content",
    "Partial Content"
};

/* 3xx -- Redirection */
#define RESPONSE_CODE_COUNT_3XX		6
static const char *response_code_messages_3xx[] = {
    "Multipul Choises",
    "Moved Permanently",
    "Moved Temporarily",
    "See Other",
    "Not Modified",
    "Use Proxy"
};

/* 4xx -- Error */
#define RESPONSE_CODE_COUNT_4XX		16
static const char *response_code_messages_4xx[] = {
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Time Out",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Request Entity Too Large",
    "Request URI Too Large",
    "Unsupported Media Type"
};

/* 5xx -- Server Error */
#define RESPONSE_CODE_COUNT_5XX		6
static const char *response_code_messages_5xx[] = {
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Time Out",
    "HTTP Version Not Supported"
};


const char *
http_response_code_message(code)
    HTTP_Response_Code code;
{
    const char *message;
    int code_type;
    int code_index;

    if (code < 100 || 600 <= code)
	return NULL;

    code_type = code / 100 * 100;
    code_index = code % 100;

    message = NULL;
    switch (code_type) {
    case 100:
	if (code_index < RESPONSE_CODE_COUNT_1XX)
	    message = response_code_messages_1xx[code_index];
	break;
    case 200:
	if (code_index < RESPONSE_CODE_COUNT_2XX)
	    message = response_code_messages_2xx[code_index];
	break;
    case 300:
	if (code_index < RESPONSE_CODE_COUNT_3XX)
	    message = response_code_messages_3xx[code_index];
	break;
    case 400:
	if (code_index < RESPONSE_CODE_COUNT_4XX)
	    message = response_code_messages_4xx[code_index];
	break;
    case 500:
	if (code_index < RESPONSE_CODE_COUNT_5XX)
	    message = response_code_messages_5xx[code_index];
	break;
    }

    return message;
}

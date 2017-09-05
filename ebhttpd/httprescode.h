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

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

/*
 * Type for HTTP response code.
 */
typedef int HTTP_Response_Code;

/*
 * HTTP response codes.
 */
/* 1xx -- Informational */
#define HTTP_RESPONSE_CONTINUE				100
#define HTTP_RESPONSE_SWITCHING_PROTOCOLS		101

/* 2xx -- Successful */
#define HTTP_RESPONSE_OK				200
#define HTTP_RESPONSE_CREATED				201
#define HTTP_RESPONSE_ACCEPTED				202
#define HTTP_RESPONSE_NON_AUTHORITATIVE_INFORMATION	203
#define HTTP_RESPONSE_NO_CONTENT			204
#define HTTP_RESPONSE_RESET_CONTENT			205
#define HTTP_RESPONSE_PARTIAL_CONTENT			206

/* 3xx -- Redirection */
#define HTTP_RESPONSE_MULTIPUL_CHOISES			300
#define HTTP_RESPONSE_MOVED_PERMANENTLY			301
#define HTTP_RESPONSE_MOVED_TEMPORARILY			302
#define HTTP_RESPONSE_SEE_OTHER				303
#define HTTP_RESPONSE_NOT_MODIFIED			304
#define HTTP_RESPONSE_USE_PROXY				305

/* 4xx -- Error */
#define HTTP_RESPONSE_BAD_REQUEST			400
#define HTTP_RESPONSE_UNAUTHORIZED			401
#define HTTP_RESPONSE_PAYMENT_REQUIRED			402
#define HTTP_RESPONSE_FORBIDDEN				403
#define HTTP_RESPONSE_NOT_FOUND				404
#define HTTP_RESPONSE_METHOD_NOT_ALLOWED		405
#define HTTP_RESPONSE_NOT_ACCEPTABLE			406
#define HTTP_RESPONSE_PROXY_AUTHENTICATION_REQUIRED	407
#define HTTP_RESPONSE_REQUEST_TIME_OUT			408
#define HTTP_RESPONSE_CONFLICT				409
#define HTTP_RESPONSE_GONE				410
#define HTTP_RESPONSE_LENGTH_REQUIRED			411
#define HTTP_RESPONSE_PRECONDITION_FAILED		412
#define HTTP_RESPONSE_REQUEST_ENTITY_TOO_LARGE		413
#define HTTP_RESPONSE_REQUEST_URI_TOO_LARGE		414
#define HTTP_RESPONSE_UNSUPPORTED_MEDIA_TYPE		415

/* 5xx -- Server Error */
#define HTTP_RESPONSE_INTERNAL_SERVER_ERROR		500
#define HTTP_RESPONSE_NOT_IMPLEMENTED			501
#define HTTP_RESPONSE_BAD_GATEWAY			502
#define HTTP_RESPONSE_SERVICE_UNAVAILABLE		503
#define HTTP_RESPONSE_GATEWAY_TIME_OUT			504
#define HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED	505

#ifdef __STDC__
const char *http_response_code_message(HTTP_Response_Code);
#else
const char *http_response_code_message();
#endif

#endif /* not HTTPRESPONSE_H */

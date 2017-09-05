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
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ioall.h"
#include "linebuf.h"
#include "pathparts.h"

#include "defs.h"
#include "cgiquery.h"
#include "httpdate.h"
#include "httprequest.h"
#include "http.h"
#include "entity.h"

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
 * Entity generator table.
 */
static const Entity_Generator entity_generator_table[] = {
    {"/",
     "text/html",
     "euc-jp",
     output_book_list
    },
    {"/*/",
     "text/html",
     "euc-jp",
     output_subbook_list
    },
    {"/*/*/",
     "text/html",
     "euc-jp",
     output_method_list
    },
    {"/*/*/word",
     "text/html",
     "euc-jp",
     output_word_form
    },
    {"/*/*/exactword",
     "text/html",
     "euc-jp",
     output_exactword_form
    },
    {"/*/*/endword",
     "text/html",
     "euc-jp",
     output_endword_form
    },
    {"/*/*/keyword",
     "text/html",
     "euc-jp",
     output_keyword_form
    },
    {"/*/*/*/multi",
     "text/html",
     "euc-jp",
     output_multi_form
    },
    {"/*/*/search-word",
     "text/html",
     "euc-jp",
     output_search_word
    },
    {"/*/*/search-exactword",
     "text/html",
     "euc-jp",
     output_search_exactword
    },
    {"/*/*/search-endword",
     "text/html",
     "euc-jp",
     output_search_endword
    },
    {"/*/*/search-keyword",
     "text/html",
     "euc-jp",
     output_search_keyword
    },
    {"/*/*/*/search-multi",
     "text/html",
     "euc-jp",
     output_search_multi
    },
    {"/*/*/text",
     "text/html",
     "euc-jp",
     output_text
    },
    {"/*/*/narrow.gif",
     "image/gif",
     "euc-jp",
     output_narrow_font_character
    },
    {"/*/*/wide.gif",
     "image/gif",
     NULL,
     output_wide_font_character
    },
    {"/*/*/mono.bmp",
     "image/x-bmp",
     NULL,
     output_mono_graphic
    },
    {"/*/*/gray.bmp",
     "image/x-bmp",
     NULL,
     output_gray_graphic
    },
    {"/*/*/color.jpg",
     "image/jpeg",
     NULL,
     output_color_graphic
    },
    {"/*/*/color.bmp",
     "image/x-bmp",
     NULL,
     output_color_graphic
    },
    {"/*/*/sound.wav",
     "audio/wave",
     NULL,
     output_wave
    },
    {"/*/*/movie.mpg",
     "video/mpeg",
     NULL,
     output_mpeg
    },
    {NULL, NULL, NULL}
};

const Entity_Generator error_entity_generator = {
    "",
    "text/html",
    "us-ascii",
    output_error_message
};

/*
 * Hookset for text.
 */
static EB_Hookset html_hookset;

/*
 * Unexported functions.
 */
static void output_response EBNETD_P((Response_Property *));
static void output_string_asis EBNETD_P((Response_Property *, const char *));
static void output_data_asis EBNETD_P((Response_Property *, const char *,
    size_t));
static void output_data_chunked EBNETD_P((Response_Property *, const char *,
    size_t));
static void output_terminator_chunk EBNETD_P((Response_Property *));
static void initialize_property EBNETD_P((Response_Property *));
static void finalize_property EBNETD_P((Response_Property *));
static const Entity_Generator *find_entity_generator EBNETD_P((const char *));
static int parse_path EBNETD_P((HTTP_Request *, Response_Property *));
static int parse_query EBNETD_P((HTTP_Request *, Response_Property *));

/*
 * Communicate with a client by HTTP.
 */
int
protocol_main()
{
    HTTP_Request request;
    HTTP_Method_Code method;
    Response_Property property;
    Line_Buffer line_buffer;
    HTTP_Status_Code request_status;
    struct tm last_modified;
    struct stat status;

    eb_initialize_hookset(&html_hookset);
    set_html_hooks(&html_hookset);

    initialize_line_buffer(&line_buffer);
    bind_file_to_line_buffer(&line_buffer, accepted_in_file);
    set_html_hooks(&html_hookset);

    /*
     * Set last-modified-time using the configuration file.
     */
    if (stat(configuration_file_name, &status) < 0)
        http_set_origin_date(&last_modified);
    else 
        memcpy(&last_modified, gmtime(&status.st_mtime), sizeof(struct tm));

    for (;;) {
	/*
	 * Initialize resources for a request.
	 */
	http_initialize_request(&request);
	initialize_property(&property);

	/*
	 * Receive an HTTP request.
	 */
	alarm(idle_timeout);
	if (http_receive_request(&request, &line_buffer) < 0)
	    goto die;

	/*
	 * Copy response-code, status in `request' to `property'.
	 */
	method = http_request_method(&request);
	if (method != HTTP_METHOD_GET && method != HTTP_METHOD_HEAD)
	    property.response_code = HTTP_RESPONSE_METHOD_NOT_ALLOWED;
	else
	    property.response_code = http_request_response_code(&request);
	property.status = http_request_status(&request);

	/*
	 * To HTTP/1.1 client, we send entity with `chunked' transfer
	 * encoding, and keep the connection unless the client claims
	 * `Connection: close'.
	 * 
	 * To HTTP/0.9 and HTTP/1.0 clients, we send entity without
	 * transfer encoding, and close after sending the entity even
	 * when an HTTP/1.0 client claims `Connection: keep-alive'.
	 */
	if (HTTP_VERSION_1_1 <= http_request_version(&request))
	    property.entity_transfer_mode = ENTITY_TRANSFER_CHUNKED;

	/*
	 * Parse the request.
	 */
	parse_path(&request, &property);
	parse_query(&request, &property);

	/*
	 * Check "If-Modified-Since".
	 */
	if (property.response_code == HTTP_RESPONSE_OK
	    && http_compare_date(&last_modified, 
		http_request_if_modified_since(&request)) <= 0) {
	    property.response_code = HTTP_RESPONSE_NOT_MODIFIED;
	    property.entity_transfer_mode = ENTITY_TRANSFER_NONE;
	}

	/*
	 * Check method "HEAD".
	 */
	if (method == HTTP_METHOD_HEAD)
	    property.entity_transfer_mode = ENTITY_TRANSFER_NONE;

	/*
	 * Output response code, headers, and entity.
	 */
	alarm(idle_timeout);
	output_response(&property);

	syslog(LOG_INFO,
	    "response: code=%03d host=%s(%s) method=%s url=%s version=HTTP/%s",
	    property.response_code, client_host_name, client_address,
	    http_request_method_string(&request), http_request_url(&request),
	    http_request_version_string(&request));

	/*
	 * Finalize resources for a request.
	 */
	request_status = property.status;
	http_finalize_request(&request);
	finalize_property(&property);

	if (request_status == HTTP_STATUS_CLOSE_AFTER_RESPOND)
	    break;
    }

    alarm(0);
    return 0;

  die:
    alarm(0);
    return -1;

}


/*
 * Output HTTP response; HTTP status line, header and entity.
 */
static void
output_response(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    char date_string[HTTP_DATE_STRING_LENGTH + 1];
    const char *response_code_message;

    /*
     * Send a response code line.
     */
    response_code_message
	= http_response_code_message(property->response_code);
    if (response_code_message == NULL)
	response_code_message = "Unknown";
    sprintf(text, "HTTP/1.1 %03d %s\r\n", property->response_code,
	response_code_message);
    output_string_asis(property, text);

    /*
     * Send HTTP header fields.
     */
    if (http_make_date(date_string) == 0) {
	sprintf(text, "Date: %s\r\n", date_string);
	output_string_asis(property, text);
    }
    sprintf(text, "Server: ebHTTPD %s\r\n", VERSION);
    output_string_asis(property, text);

    if (property->entity_generator->content_charset != NULL) {
	sprintf(text, "Content-Type: %s; charset=\"%s\"\n",
	    property->entity_generator->content_type,
	    property->entity_generator->content_charset);
    } else {
	sprintf(text, "Content-Type: %s\r\n",
	    property->entity_generator->content_type);
    }
    output_string_asis(property, text);

    if (property->entity_transfer_mode == ENTITY_TRANSFER_CHUNKED)
	output_string_asis(property, "Transfer-Encoding: chunked\r\n");

    output_string_asis(property, "\r\n");

    /*
     * Send HTTP entity.
     */
    if (property->entity_transfer_mode != ENTITY_TRANSFER_NONE) {
	property->entity_generator->function(property);
	if (property->entity_transfer_mode == ENTITY_TRANSFER_CHUNKED)
	    output_terminator_chunk(property);
    }

    finalize_property(property);
}


/*
 * Send `string' as HTTP response without transfer encoding.
 */
static void
output_string_asis(property, string)
    Response_Property *property;
    const char *string;
{
    output_data_asis(property, string, strlen(string));
}


/*
 * Send `data' as HTTP response without transfer encoding.
 */
static void
output_data_asis(property, data, data_length)
    Response_Property *property;
    const char *data;
    size_t data_length;
{
    if (property->response_error_flag)
	return;

    /*
     * Send data as it is.
     */
    if (write_all(accepted_out_file, idle_timeout, data, data_length) < 0) {
	syslog(LOG_INFO, "failed to send response, %s", strerror(errno));
	property->response_error_flag = 1;
    }
}


/*
 * Send `data' as HTTP response with "chunked" transer encoding.
 */
static void
output_data_chunked(property, data, data_length)
    Response_Property *property;
    const char *data;
    size_t data_length;
{
    /* log10(MAX_INT) + CR + LF + 1 < 16 */
    char chunk_header[16];

    if (property->response_error_flag)
	return;

    /*
     * Send data with "chunked" transfer encoding.
     */
    sprintf(chunk_header, "%x\r\n", data_length);
    if (write_string_all(accepted_out_file, idle_timeout, chunk_header) < 0
	|| write_all(accepted_out_file, idle_timeout, data, data_length) < 0
	|| write_string_all(accepted_out_file, idle_timeout, "\r\n") < 0) {
	syslog(LOG_INFO, "failed to send response, %s", strerror(errno));
	property->response_error_flag = 1;
    }
}


/*
 * Send `string' as HTTP response.
 */
void
output_string(property, string)
    Response_Property *property;
    const char *string;
{
    if (property->entity_transfer_mode == ENTITY_TRANSFER_CHUNKED)
	output_data_chunked(property, string, strlen(string));
    else
	output_data_asis(property, string, strlen(string));
}


/*
 * Send `data' as HTTP response.
 */
void
output_data(property, data, data_length)
    Response_Property *property;
    const char *data;
    size_t data_length;
{
    if (property->entity_transfer_mode == ENTITY_TRANSFER_CHUNKED)
	output_data_chunked(property, data, data_length);
    else
	output_data_asis(property, data, data_length);
}


/*
 * Encode `string' with CGI encoding, and send the result.
 */
void
output_cgi_string(property, string)
    Response_Property *property;
    const char *string;
{
    static const char hex_characters[]
	= {'0', '1', '2', '3', '4', '5', '6', '7',
	   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char encoded_string[MAX_STRING_LENGTH + 1];
    size_t encoded_length;
    unsigned char *string_p;
    unsigned char *encoded_p;

    encoded_p = (unsigned char *)encoded_string;
    encoded_length = 0;

    for (string_p = (unsigned char *)string; *string_p != '\0'; string_p++) {
	/*
	 * Send encoded string when `encoded_p' reaches to near of
	 * the end of `encoded_string'.
	 */
	if (MAX_STRING_LENGTH - 3 < encoded_length) {
	    *encoded_p = '\0';
	    output_string(property, encoded_string);
	    encoded_p = (unsigned char *)encoded_string;
	    encoded_length = 0;
	}

	/*
	 * Alphabet, digit, '$", '-', '_', and  ',' are simply copied.
	 * ' ' is converted to `+'.
	 * Other characters are converted to "% HEX HEX".
	 */
	if (isalnum(*string_p)
	    || *string_p == '$'
	    || *string_p == '-'
	    || *string_p == '_'
	    || *string_p == '.') {
	    *encoded_p++ = *string_p;
	    encoded_length++;
	} else if (*string_p == ' ') {
	    *encoded_p++ = '+';
	    encoded_length++;
	} else {
	    *encoded_p++ = '%';
	    *encoded_p++ = hex_characters[(*string_p >> 4) & 0x0f];
	    *encoded_p++ = hex_characters[(*string_p)      & 0x0f];
	    encoded_length += 3;
	}
    }

    if (0 < encoded_length) {
	*encoded_p = '\0';
	output_string(property, encoded_string);
    }
}


/*
 * Encode `string', and send the result.
 * It assumes the character code of the string is the safe as that of
 * the current book.
 */
void
output_html_safe_string(property, string)
    Response_Property *property;
    const char *string;
{
    EB_Character_Code character_code;
    EB_Error_Code error_code;

    error_code = eb_character_code(&property->book->book, &character_code);
    if (error_code == EB_SUCCESS) {
	if (character_code == EB_CHARCODE_ISO8859_1)
	    output_latin1_html_safe_string(property, string);
	else
	    output_euc_html_safe_string(property, string);
    }
}

/*
 * Encode `string' written in EUC-JP to HTML text, and send the result.
 */
void
output_euc_html_safe_string(property, string)
    Response_Property *property;
    const char *string;
{
    char encoded_string[MAX_STRING_LENGTH + 1];
    char *encoded_p;
    size_t encoded_length;
    const char *string_p;
    const char *entity;
    size_t entity_length;

    encoded_p = encoded_string;
    encoded_length = 0;

    for (string_p = string; *string_p != '\0'; string_p++) {
	/*
	 * Send encoded string when `encoded_p' reaches to near of
	 * the end of `encoded_string'.
	 */
	if (MAX_STRING_LENGTH - MAX_ENTITY_NAME_LENGTH < encoded_length) {
	    *encoded_p = '\0';
	    output_string(property, encoded_string);
	    encoded_p = encoded_string;
	    encoded_length = 0;
	}

	if (0x21 <= *string_p && *string_p <= 0x7e)
	    entity = latin1_entity_name_table[*(unsigned char *)string_p];
	else
	    entity = NULL;

	if (entity != NULL) {
	    entity_length = strlen(entity);
	    *encoded_p++ = '&';
	    memcpy(encoded_p, entity, entity_length);
	    encoded_p += entity_length;
	    *encoded_p++ = ';';
	    encoded_length += entity_length + 2;
	} else {
	    *encoded_p++ = *string_p;
	    encoded_length++;
	}
    }

    if (0 < encoded_length) {
	*encoded_p = '\0';
	output_string(property, encoded_string);
    }
}


/*
 * Encode `string' written in ISO 8859-1 to HTML text, and send the result.
 */
void
output_latin1_html_safe_string(property, string)
    Response_Property *property;
    const char *string;
{
    char encoded_string[MAX_STRING_LENGTH + 1];
    char *encoded_p;
    size_t encoded_length;
    const char *string_p;
    const char *entity;
    size_t entity_length;

    encoded_p = encoded_string;
    encoded_length = 0;

    for (string_p = string; *string_p != '\0'; string_p++) {
	/*
	 * Send encoded string when `encoded_p' reaches to near of
	 * the end of `encoded_string'.
	 */
	if (MAX_STRING_LENGTH - MAX_ENTITY_NAME_LENGTH < encoded_length) {
	    *encoded_p = '\0';
	    output_string(property, encoded_string);
	    encoded_p = encoded_string;
	    encoded_length = 0;
	}

	entity = latin1_entity_name_table[*(unsigned char *)string_p];

	if (entity != NULL) {
	    entity_length = strlen(entity);
	    *encoded_p++ = '&';
	    memcpy(encoded_p, entity, entity_length);
	    encoded_p += entity_length;
	    *encoded_p++ = ';';
	    encoded_length += entity_length + 2;
	} else {
	    *encoded_p++ = *string_p;
	    encoded_length++;
	}
    }

    if (0 < encoded_length) {
	*encoded_p = '\0';
	output_string(property, encoded_string);
    }
}


/*
 * Send a terminator chunk (zero-sized chunk).
 */
static void
output_terminator_chunk(property)
    Response_Property *property;
{
    if (property->response_error_flag)
	return;

    /*
     * Send a terminator.  Entity header is empty.
     */
    if (write_string_all(accepted_out_file, idle_timeout, "0\r\n\r\n") < 0)
	syslog(LOG_DEBUG, "send failed: (entity terminator)");
    else
	syslog(LOG_DEBUG, "send succeeded: (entity terminator)");
}


/*
 * Initialize request property.
 */
static void
initialize_property(property)
    Response_Property *property;
{
    int i;

    property->status = HTTP_STATUS_OPEN;
    property->entity_generator = NULL;
    property->entity_transfer_mode = ENTITY_TRANSFER_ASIS;
    property->response_error_flag = 0;
    property->response_code = HTTP_RESPONSE_OK;
    property->book = NULL;
    property->subbook_name[0] = '\0';
    property->subbook_title[0] = '\0';
    for (i = 0; i < EB_MAX_KEYWORDS; i++)
	property->input_words[i][0] = '\0';
    property->multi_search = EB_MULTI_INVALID;
    property->multi_entry = -1;
    property->whence = 1;
    property->position.page = 0;
    property->position.offset = 0;
    property->local_character = 0;
    property->size = 0;
    property->width = 0;
    property->height = 0;
    property->file_name[0] = '\0';
    property->hookset = &html_hookset;
}


/*
 * Finalize request property.
 */
static void
finalize_property(property)
    Response_Property *property;
{
}

/*
 * Find a command entity `command_name' in `entity_generator_table'.
 */
static const Entity_Generator *
find_entity_generator(path)
    const char *path;
{
    const Entity_Generator *command = entity_generator_table;
    const char *pattern_p;
    const char *path_p;
    int matched;

    while (command->pattern != NULL) {
	pattern_p = command->pattern;
	path_p = path;
	matched = 1;
	for (;;) {
	    if (*pattern_p != *path_p) {
		matched = 0;
		break;
	    }
	    if (*pattern_p == '\0')
		break;

	    if (*pattern_p == '/'
		&& *(pattern_p + 1) == '*'
		&& (*(pattern_p + 2) == '/' || *(pattern_p + 2) == '\0')) {
		pattern_p += 2;
		path_p++;
		while (*path_p != '/' && *path_p != '\0')
		    path_p++;
	    } else {
		pattern_p++;
		path_p++;
	    }
	}
	if (matched)
	    return command;
	command++;
    }

    return NULL;
}


/*
 * Parse the path of the received URL.
 */
static int
parse_path(request, property)
    HTTP_Request *request;
    Response_Property *property;
{
    EB_Error_Code error_code;
    const char *path_string;
    Path_Parts path_parts;
    const char *book_name;
    EB_Subbook_Code subbook;
    EB_Subbook_Code appendix_subbook;
    const char *subbook_name;
    const char *multi_search;

    /*
     * Initialize variables.
     */
    path_string = http_request_url_path(request);
    if (*path_string == '\0')
	path_string = "/";
    path_parts_initialize(&path_parts);
    if (path_parts_parse(&path_parts, path_string) < 0) {
	property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	goto failed;
    }

    /*
     * Return immediately when the response code is not OK (200).
     */
    if (property->response_code != HTTP_RESPONSE_OK)
	goto failed;
	
    /*
     * Determine entity generator.
     */
    property->entity_generator = find_entity_generator(path_string);
    if (property->entity_generator == NULL)
	goto failed;

    /*
     * Set the current book.
     */
    book_name = path_parts_segment(&path_parts, 0);
    if (book_name == NULL)
	goto succeeded;
    property->book = find_book(book_name);
    if (property->book == NULL) {
	property->response_code = HTTP_RESPONSE_NOT_FOUND;
	goto failed;
    }
    if (!property->book->permission_flag) {
	property->response_code = HTTP_RESPONSE_FORBIDDEN;
	goto failed;
    }

    /*
     * Set the current subbook.
     */
    subbook_name = path_parts_segment(&path_parts, 1);
    if (subbook_name == NULL)
	goto succeeded;
    subbook = find_subbook(property->book, subbook_name);
    appendix_subbook = find_appendix_subbook(property->book, subbook_name);
    if (subbook == EB_SUBBOOK_INVALID) {
	property->response_code = HTTP_RESPONSE_NOT_FOUND;
	goto failed;
    }
    error_code = eb_set_subbook(&property->book->book, subbook);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: book=%s", eb_error_message(error_code),
	    property->book->name);
	property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	goto failed;
    }

    /*
     * Get canonical name and title of the subbook.
     */
    error_code = eb_subbook_directory(&property->book->book,
	property->subbook_name);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: book=%s", eb_error_message(error_code),
	    property->book->name);
	property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	goto failed;
    }
    error_code = eb_subbook_title(&property->book->book,
	property->subbook_title);
    if (error_code != EB_SUCCESS) {
	syslog(LOG_INFO, "%s: book=%s", eb_error_message(error_code),
	    property->book->name);
	property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	goto failed;
    }

    /*
     * Set font of the subbook.  We ignore error.
     */
    eb_set_font(&property->book->book, EB_FONT_16);

    /*
     * Set the current subbook of the current appendix.
     */
    if (appendix_subbook != EB_SUBBOOK_INVALID) {
	error_code = eb_set_appendix_subbook(&property->book->appendix,
	    appendix_subbook);
	if (error_code != EB_SUCCESS) {
	    syslog(LOG_INFO, "%s: book=%s", eb_error_message(error_code),
		property->book->name);
	    property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	    property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	    goto failed;
	}
    }

    /*
     * Set multi search.
     */
    multi_search = path_parts_segment(&path_parts, 2);
    if (multi_search != NULL)
	property->multi_search = atoi(multi_search);

  succeeded:
    path_parts_finalize(&path_parts);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    path_parts_finalize(&path_parts);
    if (property->response_code == HTTP_RESPONSE_OK)
	property->response_code = HTTP_RESPONSE_NOT_FOUND;
    property->entity_generator = &error_entity_generator;

    return -1;
}


/*
 * Get query elements from a CGI query string.
 */
static int
parse_query(request, property)
    HTTP_Request *request;
    Response_Property *property;
{
    const char *query_string;
    CGI_Query cgi_query;
    char query_name[16];
    int i;

    /*
     * Initialize variables.
     */
    query_string = http_request_url_query(request);
    cgi_query_initialize(&cgi_query);
    if (cgi_query_parse_string(&cgi_query, query_string) < 0) {
	property->response_code = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
	property->status = HTTP_STATUS_CLOSE_AFTER_RESPOND;
	goto failed;
    }

    /*
     * Return immediately when the response code is not OK (200).
     */
    if (property->response_code != HTTP_RESPONSE_OK)
	goto failed;
	
    /*
     * Get elements `word1', `word2', ....
     */
    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	sprintf(query_name, "word%d", i + 1);
	strncpy(property->input_words[i],
	    cgi_query_element_string_value(&cgi_query, query_name),
	    EB_MAX_WORD_LENGTH);
	property->input_words[i][EB_MAX_WORD_LENGTH] = '\0';
    }

    /*
     * Get elements `select*'.
     */
    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	sprintf(query_name, "select%d", i + 1);
	if (cgi_query_element_value(&cgi_query, query_name) != NULL) {
	    property->multi_entry = i;
	    break;
	}
    }
    
    /*
     * Get other elements.
     */
    property->whence
	= cgi_query_element_integer_value(&cgi_query, "whence");
    property->position.page
	= cgi_query_element_integer_value(&cgi_query, "page");
    property->position.offset
	= cgi_query_element_integer_value(&cgi_query, "offset");
    property->size
	= cgi_query_element_integer_value(&cgi_query, "size");
    property->width
	= cgi_query_element_integer_value(&cgi_query, "width");
    property->height
	= cgi_query_element_integer_value(&cgi_query, "height");
    property->local_character
	= cgi_query_element_integer_value(&cgi_query, "char");

    strncpy(property->file_name,
	cgi_query_element_string_value(&cgi_query, "filename"),
	EB_MAX_DIRECTORY_NAME_LENGTH);
    property->file_name[EB_MAX_DIRECTORY_NAME_LENGTH] = '\0';

    /*
     * Finalize variables.
     */
    cgi_query_finalize(&cgi_query);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    cgi_query_finalize(&cgi_query);
    return -1;
}

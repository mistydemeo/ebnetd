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

#ifndef HTTP_H
#define HTTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <eb/eb.h>
#include <eb/error.h>
#include <eb/appendix.h>
#include <eb/binary.h>
#include <eb/text.h>
#include <eb/font.h>

#include "defs.h"
#include "httprequest.h"

/*
 * Default charset of text content.
 */
#define DEFAULT_CONTENT_CHARSET		"euc-jp"

/*
 * Whether response entity is sent or not, and if sent, what transfer
 * encoding will be used (`chunked' or `asis').
 */
#define ENTITY_TRANSFER_NONE		0
#define ENTITY_TRANSFER_ASIS		1
#define ENTITY_TRANSFER_CHUNKED		2

/*
 * Type definitions.
 */
typedef struct Entity_Generator_Struct Entity_Generator;
typedef struct Response_Property_Struct Response_Property;

/*
 * Entity generator.
 */
struct Entity_Generator_Struct {
    const char *pattern;
    const char *content_type;
    const char *content_charset;
    void (*function) EBNETD_P((Response_Property *));
};

/*
 * Various properties for generating response entity.
 */
struct Response_Property_Struct {
    /*
     * Parser status.
     */
    HTTP_Status_Code status;

    /*
     * Entity generator.
     */
    const Entity_Generator *entity_generator;

    /*
     * Whether response entity is sent or not, and if sent, what
     * transfer encoding will be used.
     */
    int entity_transfer_mode;

    /*
     * Whether an error occured in sending response data.
     */
    int response_error_flag;

    /*
     * HTTP response code.
     */
    int response_code;

    /*
     * Current book.
     */
    Book *book;

    /*
     * Current subbook name.
     */
    char subbook_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * Current subbook title.
     */
    char subbook_title[EB_MAX_TITLE_LENGTH + 1];

    /*
     * Current input words.
     * Represented as `word1', `word2' ... in CGI query.
     */
    char input_words[EB_MAX_KEYWORDS][EB_MAX_WORD_LENGTH + 1];

    /*
     * Current multi search.
     */
    EB_Multi_Search_Code multi_search;

    /*
     * Current multi search entry.
     * Represented as `select' in CGI query.
     */
    EB_Multi_Entry_Code multi_entry;

    /*
     * Current whence of the hit entries.
     * Represented as `whence' in CGI query.
     */
    int whence;

    /*
     * Current text page and offset.
     * Represented as `page' and `offset' in CGI query.
     */
    EB_Position position;

    /*
     * Local character number.
     * Represented as `char' in CGI query.
     */
    int local_character;

    /*
     * Data size for WAVE sound data.
     * Represented as `size' in CGI query.
     */
    size_t size;

    /*
     * Width for monochrome graphic data.
     * Represented as `width' in CGI query.
     */
    size_t width;

    /*
     * Width for monochrome graphic data.
     * Represented as `width' in CGI query.
     */
    size_t height;

    /*
     * Data size for MPEG movie data.
     * Represented as `filename' in CGI query.
     */
    char file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * Hookset to generate HTML.
     */
    EB_Hookset *hookset;

};

/*
 * Function Declaration.
 */
/* hookest.c */
void set_html_hooks EBNETD_P((EB_Hookset *));
/* http.c */
void output_string EBNETD_P((Response_Property *, const char *));
void output_data EBNETD_P((Response_Property *, const char *, size_t));
void output_cgi_string EBNETD_P((Response_Property *, const char *));
void output_html_safe_string EBNETD_P((Response_Property *, const char *));
void output_euc_html_safe_string EBNETD_P((Response_Property *, const char *));
void output_latin1_html_safe_string EBNETD_P((Response_Property *,
    const char *));
/* generator.c */
void output_error_message EBNETD_P((Response_Property *));
void output_book_list EBNETD_P((Response_Property *));
void output_subbook_list EBNETD_P((Response_Property *));
void output_method_list EBNETD_P((Response_Property *));
void output_word_form EBNETD_P((Response_Property *));
void output_exactword_form EBNETD_P((Response_Property *));
void output_endword_form EBNETD_P((Response_Property *));
void output_keyword_form EBNETD_P((Response_Property *));
void output_multi_form EBNETD_P((Response_Property *));
void output_search_word EBNETD_P((Response_Property *));
void output_search_exactword EBNETD_P((Response_Property *));
void output_search_endword EBNETD_P((Response_Property *));
void output_search_keyword EBNETD_P((Response_Property *));
void output_search_multi EBNETD_P((Response_Property *));
void output_text EBNETD_P((Response_Property *));
void output_narrow_font_character EBNETD_P((Response_Property *));
void output_wide_font_character EBNETD_P((Response_Property *));
void output_mono_graphic EBNETD_P((Response_Property *));
void output_gray_graphic EBNETD_P((Response_Property *));
void output_color_graphic EBNETD_P((Response_Property *));
void output_wave EBNETD_P((Response_Property *));
void output_mpeg EBNETD_P((Response_Property *));

#endif /* not HTTP_H */

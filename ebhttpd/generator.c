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

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

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

#include <eb/error.h>
#include <eb/eb.h>
#include <eb/appendix.h>
#include <eb/font.h>
#include <eb/text.h>
#include <eb/binary.h>

#include "httprescode.h"
#include "http.h"

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

#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif

/*
 * Width of the text entry in a form.
 */
#define INPUT_TEXT_ENTRY_SIZE	40

/*
 * Unexported functions.
 */
static void output_html_header EBNETD_P((Response_Property *, const char *));
static void output_html_footer EBNETD_P((Response_Property *));
static void output_word_list EBNETD_P((Response_Property *));
static void output_hit_list EBNETD_P((Response_Property *, int *, int *));
static void output_search_mutli_select EBNETD_P((Response_Property *));
static void output_search_mutli_result EBNETD_P((Response_Property *));
static void output_text_internal EBNETD_P((Response_Property *, EB_Position *));


/*
 * Output header part of HTML.
 */
static void
output_html_header(property, title)
    Response_Property *property;
    const char *title;
{
    char text[MAX_STRING_LENGTH + 1];

    output_string(property,
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n");
    output_string(property, "<html>\n");
    output_string(property, "<head>\n");
    sprintf(text, "<title>%s</title>\n", title);
    output_string(property, text);
    output_string(property, "</head>\n");
    output_string(property, "<body>\n");
    sprintf(text, "<h1>%s</h1>\n", title);
    output_string(property, text);
}


/*
 * Output footer part of HTML.
 */
static void
output_html_footer(property)
    Response_Property *property;
{
    output_string(property, "<hr>\n");
    output_string(property, "<address>Powered by ebHTTPD</address>\n");
    output_string(property, "</body>\n");
    output_string(property, "</html>\n");
}


/*
 * Output error message as HTML.
 */
void
output_error_message(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    const char *response_code_message;

    output_html_header(property, "Error");

    response_code_message 
	= http_response_code_message(property->response_code);
    if (response_code_message == NULL)
	response_code_message = "Unknown Error";
    sprintf(text, "%03d %s\n", property->response_code, response_code_message);
    output_string(property, text);
    output_html_footer(property);
}


/*
 * Output `&word1=...&word2=....&...' as HTML.
 */
static void
output_word_list(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    int i;

    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	sprintf(text, "&amp;word%d=", i + 1);
	output_string(property, text);
	output_cgi_string(property, property->input_words[i]);
    }
}

/*
 * Output book list as HTML.
 * (path = /)
 */
void
output_book_list(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    Book *book;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "書籍一覧");

    /*
     * Output a book list.
     */
    output_string(property, "<ul>\n");
    for (book = book_registry; book != NULL; book = book->next) {
        if (!eb_is_bound(&book->book))
            continue;
	sprintf(text, "<li><a href=\"/%s/\">", book->name);
	output_string(property, text);
	output_euc_html_safe_string(property, book->title);
	output_string(property, "</a>\n");
    }
    output_string(property, "</ul>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}


/*
 * Output subbook list as HTML.
 * (path = /<book-name>/)
 */
void
output_subbook_list(property)
    Response_Property *property;
{
    EB_Error_Code error_code;
    char text[MAX_STRING_LENGTH + 1];
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    char subbook_title[EB_MAX_TITLE_LENGTH + 1];
    char subbook_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_count;
    int i;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "副本一覧");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    output_string(property, "→");
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧\n");
    output_string(property, ")</p>\n");

    /*
     * Get a subbook list of the current book.
     */
    error_code = eb_subbook_list(&property->book->book, subbook_list,
	&subbook_count);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output an input form.
     */
    output_string(property, "<ul>\n");
    for (i = 0; i < subbook_count; i++) {
	error_code = eb_subbook_directory2(&property->book->book,
	    subbook_list[i], subbook_name);
	if (error_code != EB_SUCCESS)
	    continue;
	error_code = eb_subbook_title2(&property->book->book,
	    subbook_list[i], subbook_title);
	if (error_code != EB_SUCCESS)
	    continue;
	sprintf(text,"<li><a href=\"/%s/%s/\">",
	    property->book->name, subbook_name);
	output_string(property, text);
	output_string(property, subbook_title);
	output_string(property, "</a>\n");
    }
    output_string(property, "</ul>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output method list as HTML.
 * (path = /<book-name>/<subbook-name>/)
 */
void
output_method_list(property)
    Response_Property *property;
{
    EB_Error_Code error_code;
    char multi_title[EB_MAX_MULTI_TITLE_LENGTH + 1];
    char text[MAX_STRING_LENGTH + 1];
    EB_Multi_Search_Code multi_list[EB_MAX_MULTI_SEARCHES];
    EB_Position position;
    int multi_count;
    int i;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索方式一覧");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    output_string(property, "→");
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧\n");
    output_string(property, ")</p>\n");

    /*
     * Get a mulit search list of the current subbook.
     */
    eb_multi_search_list(&property->book->book, multi_list, &multi_count);

    /*
     * Output a method list.
     */
    output_string(property, "<ul>\n");

    if (eb_have_word_search(&property->book->book)) {
	sprintf(text, "<li><a href=\"/%s/%s/word\">前方一致検索</a>\n", 
	    property->book->name, property->subbook_name);
	output_string(property, text);
    }

    if (eb_have_exactword_search(&property->book->book)) {
	sprintf(text,
	    "<li><a href=\"/%s/%s/exactword\">完全一致検索</a>\n", 
	    property->book->name, property->subbook_name);
	output_string(property, text);
    }

    if (eb_have_endword_search(&property->book->book)) {
	sprintf(text, "<li><a href=\"/%s/%s/endword\">後方一致検索</a>\n", 
	    property->book->name, property->subbook_name);
	output_string(property, text);
    }

    if (eb_have_keyword_search(&property->book->book)) {
	sprintf(text, "<li><a href=\"/%s/%s/keyword\">条件検索</a>\n", 
	    property->book->name, property->subbook_name);
	output_string(property, text);
    }

    for (i = 0; i < multi_count; i++) {
	if (eb_multi_title(&property->book->book, multi_list[i], multi_title)
	    != EB_SUCCESS) {
	    sprintf(multi_title, "複合検索 %d", i);
	}
	sprintf(text,
	    "<li><a href=\"/%s/%s/%d/multi\">%s</a>\n", 
	    property->book->name, property->subbook_name, multi_list[i],
	    multi_title);
	output_string(property, text);
    }

    error_code = eb_menu(&property->book->book, &position);
    if (error_code == EB_SUCCESS) {
	sprintf(text, "<li><a href=\"/%s/%s/text?page=0x%x&amp;offset=0x%x\"",
	    property->book->name, property->subbook_name, 
	    position.page, position.offset);
	output_string(property, text);
	output_string(property,">メニュー</a>\n");
    }

    error_code = eb_copyright(&property->book->book, &position);
    if (error_code == EB_SUCCESS) {
	sprintf(text, "<li><a href=\"/%s/%s/text?page=0x%x&amp;offset=0x%x\"",
	    property->book->name, property->subbook_name,
	    position.page, position.offset);
	output_string(property, text);
	output_string(property, ">著作権表示</a>\n");
    }

    output_string(property, "</ul>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}


/*
 * Output word search input form as HTML.
 * (path = /<book-name>/<subbook-name>/word)
 */
void
output_word_form(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "前方一致検索");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    output_string(property, "→前方一致検索\n");
    output_string(property, ")</p>\n");

    /*
     * Output an input form.
     */
    sprintf(text, "<form method=\"GET\" action=\"/%s/%s/search-word\">\n",
	property->book->name, property->subbook_name);
    output_string(property, text);

    output_string(property, "<p>\n");
    output_string(property, "単語: <br>\n");
    sprintf(text,
	"<input type=\"text\" name=\"word1\" size=\"%d\" value=\"%s\"><br>\n",
	INPUT_TEXT_ENTRY_SIZE, property->input_words[0]);
    output_string(property, text);
    output_string(property, "</p>\n");

    output_string(property, "<p>\n");
    output_string(property, "<input type=\"submit\" name=\"submit\" ");
    output_string(property, "value=\"検索\">\n");
    output_string(property, "<input type=\"reset\" name=\"reset\" ");
    output_string(property, "value=\"クリア\">\n");
    output_string(property, "</p>\n");

    output_string(property, "</form>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}


/*
 * Output word search input form as HTML.
 * (path = /<book-name>/<subbook-name>/exactword)
 */
void
output_exactword_form(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "完全一致検索");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    output_string(property, "→完全一致検索\n");
    output_string(property, ")</p>\n");

    /*
     * Output an input form.
     */
    sprintf(text, "<form method=\"GET\" action=\"/%s/%s/search-exactword\">\n",
	property->book->name, property->subbook_name);
    output_string(property, text);

    output_string(property, "<p>\n");
    sprintf(text, "単語: <br>\n");
    output_string(property, text);
    sprintf(text,
	"<input type=\"text\" name=\"word1\" size=\"%d\" value=\"%s\"><br>\n",
	INPUT_TEXT_ENTRY_SIZE, property->input_words[0]);
    output_string(property, text);
    output_string(property, "</p>\n");

    output_string(property, "<p>\n");
    output_string(property, "<input type=\"submit\" name=\"submit\" ");
    output_string(property, "value=\"検索\">\n");
    output_string(property, "<input type=\"reset\" name=\"reset\" ");
    output_string(property, "value=\"クリア\">\n");
    output_string(property, "</p>\n");

    output_string(property, "</form>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}


/*
 * Output word search input form as HTML.
 * (path = /<book-name>/<subbook-name>/endword)
 */
void
output_endword_form(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "後方一致検索");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    output_string(property, "→後方一致検索\n");
    output_string(property, ")</p>\n");

    /*
     * Output an input form.
     */
    sprintf(text, "<form method=\"GET\" action=\"/%s/%s/search-endword\">\n",
	property->book->name, property->subbook_name);
    output_string(property, text);

    output_string(property, "<p>\n");
    sprintf(text, "単語: <br>\n");
    output_string(property, text);
    sprintf(text,
	"<input type=\"text\" name=\"word1\" size=\"%d\" value=\"%s\"><br>\n",
	INPUT_TEXT_ENTRY_SIZE, property->input_words[0]);
    output_string(property, text);
    output_string(property, "</p>\n");

    output_string(property, "<p>\n");
    output_string(property, "<input type=\"submit\" name=\"submit\" ");
    output_string(property, "value=\"検索\">\n");
    output_string(property, "<input type=\"reset\" name=\"reset\" ");
    output_string(property, "value=\"クリア\">\n");
    output_string(property, "</p>\n");

    output_string(property, "</form>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}


/*
 * Output keyword search input form as HTML.
 * (path = /<book-name>/<subbook-name>/keyword)
 */
void
output_keyword_form(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    int i;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "条件検索");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    output_string(property, "→条件検索\n");
    output_string(property, ")</p>\n");

    /*
     * Output an input form.
     */
    sprintf(text, "<form method=\"GET\" action=\"/%s/%s/search-keyword\">\n",
	property->book->name, property->subbook_name);
    output_string(property, text);

    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	output_string(property, "<p>\n");
	sprintf(text, "単語 %d: <br>\n", i + 1);
	output_string(property, text);
	output_string(property, "<input type=\"text\" ");
	sprintf(text, "name=\"word%d\" size=\"%d\" value=\"%s\"><br>\n",
	    i + 1, INPUT_TEXT_ENTRY_SIZE, property->input_words[i]);
	output_string(property, text);
	output_string(property, "</p>\n");
    }

    output_string(property, "<p>\n");
    output_string(property, "<input type=\"submit\" name=\"submit\" ");
    output_string(property, "value=\"検索\">\n");
    output_string(property, "<input type=\"reset\" name=\"reset\" ");
    output_string(property, "value=\"クリア\">\n");
    output_string(property, "</p>\n");

    output_string(property, "</form>\n");

    /*
     * Oputput an HTML footer.
     */
    output_html_footer(property);
}

/*
 * Output multi search input form HTML.
 * (path = /<book-name>/<subbook-name>/multi)
 */
void
output_multi_form(property)
    Response_Property *property;
{
    EB_Error_Code error_code;
    char multi_title[EB_MAX_MULTI_TITLE_LENGTH + 1];
    char text[MAX_STRING_LENGTH + 1];
    int entry_count;
    char entry_labels[EB_MAX_MULTI_ENTRIES][EB_MAX_MULTI_LABEL_LENGTH + 1];
    EB_Position candidates_positions[EB_MAX_MULTI_ENTRIES];
    int i;

    if (eb_multi_title(&property->book->book, property->multi_search,
	multi_title) != EB_SUCCESS) {
	sprintf(multi_title, "複合検索 %d", property->multi_search);
    }

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, multi_title);

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→%s\n", multi_title);
    output_string(property, text);
    output_string(property, ")</p>\n");

    /*
     * Get the number of entries of the current multi search.
     */
    error_code = eb_multi_entry_count(&property->book->book,
	property->multi_search, &entry_count);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Get an entry labels of the current multi search.
     */
    for (i = 0; i < entry_count; i++) {
	error_code = eb_multi_entry_label(&property->book->book,
	    property->multi_search, i, entry_labels[i]);
	if (error_code != EB_SUCCESS) {
	    sprintf(text, "%s\n", eb_error_message(error_code));
	    output_string(property, text);
	    goto footer;
	}
    }

    /*
     * Get candidate information of the current multi search.
     */
    for (i = 0; i < entry_count; i++) {
	error_code = eb_multi_entry_candidates(&property->book->book,
	    property->multi_search, i, &candidates_positions[i]);
	if (error_code == EB_ERR_NO_CANDIDATES) {
	    candidates_positions[i].page = 0;
	    candidates_positions[i].offset = 0;
	} else if (error_code != EB_SUCCESS) {
	    sprintf(text, "%s\n", eb_error_message(error_code));
	    output_string(property, text);
	    goto footer;
	}
    }

    /*
     * Output an input form.
     */
    sprintf(text, "<form method=\"GET\" action=\"/%s/%s/%d/search-multi\">\n",
	property->book->name, property->subbook_name, property->multi_search);
    output_string(property, text);

    for (i = 0; i < entry_count; i++) {
	output_string(property, "<p>\n");
	output_html_safe_string(property, entry_labels[i]);
	output_string(property, ":<br>\n");
	sprintf(text,
	    "<input type=\"text\" name=\"word%d\" size=\"%d\" value=\"%s\">\n",
	    i + 1, INPUT_TEXT_ENTRY_SIZE, property->input_words[i]);
	output_string(property, text);
	if (candidates_positions[i].page != 0) {
	    output_string(property, "<input type=\"submit\" ");
	    sprintf(text, "name=\"select%d\" value=\"選択...\">\n", i + 1);
	    output_string(property, text);
	}
	output_string(property, "<br>\n");
	output_string(property, "</p>\n");
    }

    output_string(property, "<p>\n");
    sprintf(text, "<input type=\"hidden\" name=\"multi\" value=\"%d\">\n",
	property->multi_search);
    output_string(property, text);
    output_string(property, "<input type=\"submit\" name=\"submit\" ");
    output_string(property, "value=\"検索\">\n");
    output_string(property, "<input type=\"reset\" name=\"reset\" ");
    output_string(property, "value=\"クリア\">\n");
    output_string(property, "</p>\n");

    output_string(property, "</form>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output word search result as HTML.
 * (path = /<book-name>/<subbook-name>/search-word)
 */
void
output_search_word(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    EB_Error_Code error_code;
    int next_whence;
    int previous_whence;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索結果");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/word\">前方一致検索</a>\n",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_string(property, "→検索結果\n");
    output_string(property, ")</p>\n");

    /*
     * Submit a search.
     */
    error_code = eb_search_word(&property->book->book,
	property->input_words[0]);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output hit list.
     */
    output_hit_list(property, &next_whence, &previous_whence);

    /*
     * Generate [前へ] and [次へ] buttons.
     */
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "<p>\n");
    if (previous_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-word?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, previous_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property,
	    "&amp;submit=submit\">[前へ]</a>\n");
    }
    if (next_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-word?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, next_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property, "&amp;submit=submit\">[次へ]</a>\n");
    }
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "</p>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output end-word search result as HTML.
 * (path = /<book-name>/<subbook-name>/search-exactword)
 */
void
output_search_exactword(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    EB_Error_Code error_code;
    int next_whence;
    int previous_whence;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索結果");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/exactword\">完全一致検索</a>\n",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_string(property, "→検索結果\n");
    output_string(property, ")</p>\n");

    /*
     * Submit a search.
     */
    error_code = eb_search_exactword(&property->book->book,
	property->input_words[0]);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output hit list.
     */
    output_hit_list(property, &next_whence, &previous_whence);

    /*
     * Generate [前へ] and [次へ] buttons.
     */
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "<p>\n");
    if (previous_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-exactword?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, previous_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property,
	    "&amp;submit=submit\">[前へ]</a>\n");
    }
    if (next_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-exactword?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, next_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property, "&amp;submit=submit\">[次へ]</a>\n");
    }
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "</p>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output end-word search result as HTML.
 * (path = /<book-name>/<subbook-name>/search-endword)
 */
void
output_search_endword(property)
    Response_Property *property;
{
    char text[MAX_STRING_LENGTH + 1];
    EB_Error_Code error_code;
    int next_whence;
    int previous_whence;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索結果");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/endword\">後方一致検索</a>\n",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_string(property, "→検索結果\n");
    output_string(property, ")</p>\n");

    /*
     * Submit a search.
     */
    error_code = eb_search_endword(&property->book->book,
	property->input_words[0]);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output hit list.
     */
    output_hit_list(property, &next_whence, &previous_whence);

    /*
     * Generate [前へ] and [次へ] buttons.
     */
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "<p>\n");
    if (previous_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-endword?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, previous_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property,
	    "&amp;submit=submit\">[前へ]</a>\n");
    }
    if (next_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-endword?whence=%d&amp;word1=",
	    property->book->name, property->subbook_name, next_whence);
	output_string(property, text);
	output_cgi_string(property, property->input_words[0]);
	output_string(property, "&amp;submit=submit\">[次へ]</a>\n");
    }
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "</p>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output keyword search result as HTML.
 * (path = /<book-name>/<subbook-name>/search-keyword)
 */
void
output_search_keyword(property)
    Response_Property *property;
{
    char *keywords[EB_MAX_KEYWORDS + 1];
    char text[MAX_STRING_LENGTH + 1];
    EB_Error_Code error_code;
    int next_whence;
    int previous_whence;
    int i;

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索結果");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/keyword\">条件一致検索</a>\n",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_string(property, "→検索結果\n");
    output_string(property, ")</p>\n");

    /*
     * Submit a search.
     */
    for (i = 0; i < EB_MAX_KEYWORDS; i++)
	keywords[i] = property->input_words[i];
    keywords[i] = NULL;
    error_code = eb_search_keyword(&property->book->book,
	(const char * const *)keywords);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output hit list.
     */
    output_hit_list(property, &next_whence, &previous_whence);

    /*
     * Generate [前へ] and [次へ] buttons.
     */
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "<p>\n");
    if (previous_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-keyword?whence=%d",
	    property->book->name, property->subbook_name, previous_whence);
	output_string(property, text);
	output_word_list(property);
	output_string(property,
	    "&amp;submit=submit\">[前へ]</a>\n");
    }
    if (next_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/search-keyword?whence=%d",
	    property->book->name, property->subbook_name, next_whence);
	output_string(property, text);
	output_word_list(property);
	output_string(property, "&amp;submit=submit\">[次へ]</a>\n");
    }
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "</p>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * Output result of multi search or candidates for multi search.
 * (path = /<book-name>/<subbook-name>/<muliti-search-id>/search-multi)
 */
void
output_search_multi(property)
    Response_Property *property;
{
    if (0 <= property->multi_entry)
	output_search_mutli_select(property);
    else
	output_search_mutli_result(property);
}


/*
 * Output candidates of multi search.
 */
static void
output_search_mutli_select(property)
    Response_Property *property;
{
    EB_Error_Code error_code;
    char multi_title[EB_MAX_MULTI_TITLE_LENGTH + 1];
    char text[MAX_STRING_LENGTH + 1];
    EB_Position *position;
    EB_Position candidates_position;

    if (eb_multi_title(&property->book->book, property->multi_search,
	multi_title) != EB_SUCCESS) {
	sprintf(multi_title, "複合検索 %d", property->multi_search);
    }

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "候補選択");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/%d/multi\">%s</a>\n",
	property->book->name, property->subbook_name, property->multi_search,
	multi_title);
    output_string(property, text);
    output_string(property, "→候補選択\n");
    output_string(property, ")</p>\n");

    /*
     * Galcurate position of candidates list.
     */
    if (0 < property->position.page)
	position = &property->position;
    else {
	error_code = eb_multi_entry_candidates(&property->book->book,
	    property->multi_search, property->multi_entry,
	    &candidates_position);
	if (error_code != EB_SUCCESS) {
	    sprintf(text, "%s\n", eb_error_message(error_code));
	    output_string(property, text);
	    goto footer;
	}
	position = &candidates_position;
    }

    /*
     * Output text.
     */
    output_string(property, "<p>");
    output_text_internal(property, position);
    output_string(property, "</p>");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}

/*
 * Output multi search result as HTML.
 */
static void
output_search_mutli_result(property)
    Response_Property *property;
{
    char *keywords[EB_MAX_KEYWORDS + 1];
    char multi_title[EB_MAX_MULTI_TITLE_LENGTH + 1];
    char text[MAX_STRING_LENGTH + 1];
    int entry_count;
    EB_Error_Code error_code;
    int next_whence;
    int previous_whence;
    int i;

    if (eb_multi_title(&property->book->book, property->multi_search,
	multi_title) != EB_SUCCESS) {
	sprintf(multi_title, "複合検索 %d", property->multi_search);
    }

    /*
     * Oputput an HTML header.
     */
    output_html_header(property, "検索結果");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/%d/multi\">%s</a>\n",
	property->book->name, property->subbook_name, property->multi_search,
	multi_title);
    output_string(property, text);
    output_string(property, "→検索結果\n");
    output_string(property, ")</p>\n");

    /*
     * Get information about multi search entries.
     */
    error_code = eb_multi_entry_count(&property->book->book,
	property->multi_search, &entry_count);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Submit a search.
     */
    for (i = 0; i < entry_count; i++)
	keywords[i] = property->input_words[i];
    keywords[i] = NULL;
    error_code = eb_search_multi(&property->book->book, property->multi_search,
	(const char * const *)keywords);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	goto footer;
    }

    /*
     * Output hit list.
     */
    output_hit_list(property, &next_whence, &previous_whence);

    /*
     * Generate [前へ] and [次へ] buttons.
     */
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "<p>\n");
    if (previous_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/%d/search-multi?whence=%d",
	    property->book->name, property->subbook_name, 
	    property->multi_search, previous_whence);
	output_string(property, text);
	output_word_list(property);
	output_string(property,
	    "&amp;submit=submit\">[前へ]</a>\n");
    }
    if (next_whence != 0) {
	sprintf(text, "<a href=\"/%s/%s/%d/search-multi?whence=%d",
	    property->book->name, property->subbook_name, 
	    property->multi_search, next_whence);
	output_string(property, text);
	output_word_list(property);
	output_string(property, "&amp;submit=submit\">[次へ]</a>\n");
    }
    if (previous_whence != 0 || next_whence != 0)
	output_string(property, "</p>\n");

    /*
     * Oputput an HTML footer.
     */
  footer:
    output_html_footer(property);
}


/*
 * The maximum number of hit entries for tomporary hit lists.
 * This is used in eb_hit_list().
 */
#define HIT_LIST_LENGTH	50

static void
output_hit_list(property, next_whence, previous_whence)
    Response_Property *property;
    int *next_whence;
    int *previous_whence;
{
    char text[MAX_STRING_LENGTH + 1];
    EB_Error_Code error_code;
    EB_Hit hit_list[HIT_LIST_LENGTH];
    char heading_buffer1[MAX_STRING_LENGTH + 1];
    char heading_buffer2[MAX_STRING_LENGTH + 1];
    char *heading;
    char *previous_heading;
    EB_Position previous_position;
    ssize_t heading_length;
    int hit_count;
    int total_hit_count = 0;
    int whence;
    int i;

    heading = heading_buffer1;
    previous_heading = heading_buffer2;
    *previous_heading = '\0';
    previous_position.page = 0;
    previous_position.offset = 0;
    whence = 1;
    *next_whence = 0;

    output_string(property, "<ul>\n");

    /*
     * Discard hit entries in front of `property->whence'.
     */
    while (whence < property->whence) {
	if (whence + HIT_LIST_LENGTH <= property->whence) {
	    error_code = eb_hit_list(&property->book->book, HIT_LIST_LENGTH,
		hit_list, &hit_count);
	} else {
	    error_code = eb_hit_list(&property->book->book,
		property->whence - whence, hit_list, &hit_count);
	}
	if (error_code != EB_SUCCESS || hit_count == 0)
	    break;
	whence += hit_count;
    }

    i = 0;
    hit_count = 0;
    while (total_hit_count < max_hits || max_hits == 0) {
        /*
         * Get hit entries.
         */
	i++;
	if (hit_count <= i) {
	    error_code = eb_hit_list(&property->book->book, HIT_LIST_LENGTH,
		hit_list, &hit_count);
	    if (error_code != EB_SUCCESS || hit_count == 0)
		break;
	    i = 0;
	}

	/*
	 * Seek the current subbook.
	 */
	error_code = eb_seek_text(&property->book->book, 
	    &hit_list[i].heading);
	if (error_code != EB_SUCCESS)
	    continue;
	    
	/*
	 * Get heading.
	 */
	error_code = eb_read_heading(&property->book->book,
	    &property->book->appendix, property->hookset, (VOID *)property,
	    MAX_STRING_LENGTH, heading, &heading_length);
	if (error_code != EB_SUCCESS || heading_length == 0)
	    continue;
	*(heading + heading_length) = '\0';

	/*
	 * Ignore a hit entry if its heading and text location are
	 * equal to those of the previous hit entry.
	 */
	if (previous_position.page == hit_list[i].text.page
	    && previous_position.offset == hit_list[i].text.offset 
	    && strcmp(heading, previous_heading) == 0)
	    continue;

	total_hit_count++;

	/*
	 * Output the heading.
	 */
	sprintf(text,
	    "<li>%d: <a href=\"/%s/%s/text?page=0x%x&amp;offset=0x%x\">",
	    whence, property->book->name, property->subbook_name,
	    hit_list[i].text.page, hit_list[i].text.offset);
	output_string(property, text);
	output_string(property, heading);
	output_string(property, "</a>\n");

	/*
	 * Keep the last message, page, and offset, to remove
	 * duplicated entries.
	 */
	if (heading == heading_buffer1) {
	    heading = heading_buffer2;
	    previous_heading = heading_buffer1;
	} else {
	    heading = heading_buffer1;
	    previous_heading = heading_buffer2;
	}
	previous_position.page = hit_list[i].text.page;
	previous_position.offset = hit_list[i].text.offset;
       
	whence++;
    }

    output_string(property, "</ul>\n");

    /*
     * Get one more hit entry in order to set `next_whence'.
     */
    if (max_hits <= total_hit_count && max_hits != 0) {
	error_code = eb_hit_list(&property->book->book, 1, hit_list,
	    &hit_count);
	if (0 < hit_count)
	    *next_whence = whence;
    }

    /*
     * Set `previous_whence'.
     */
    if (1 < property->whence)
	*previous_whence = property->whence - max_hits;
    else
	*previous_whence = 0;
}


/*
 * Output text as HTML.
 * (path = /<book-name>/<subbook-name>/text)
 */
void
output_text(property)
    Response_Property *property;
{
    char text[EB_SIZE_PAGE];

    output_html_header(property, "本文");

    output_string(property, "<p>(\n");
    output_string(property, "→<a href=\"/\">書籍一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/\">", property->book->name);
    output_string(property, text);
    output_html_safe_string(property, property->book->title);
    output_string(property, ":副本一覧</a>\n");
    sprintf(text, "→<a href=\"/%s/%s/\">",
	property->book->name, property->subbook_name);
    output_string(property, text);
    output_html_safe_string(property, property->subbook_title);
    output_string(property, ":検索方式一覧</a>\n");
    output_string(property, "→本文\n");
    output_string(property, ")</p>\n");

    output_string(property, "<p>");
    output_text_internal(property, &property->position);
    output_string(property, "</p>");
    output_html_footer(property);
}


/*
 * Internal function for output_text() and output_search_multi_select().
 */
static void
output_text_internal(property, position)
    Response_Property *property;
    EB_Position *position;
{
    char text[EB_SIZE_PAGE];
    EB_Error_Code error_code;
    size_t rest_text_length;
    size_t text_length_argument;
    ssize_t actual_text_length;

    /*
     * Seek the current subbook.
     */
    error_code = eb_seek_text(&property->book->book, position);
    if (error_code != EB_SUCCESS) {
	sprintf(text, "%s\n", eb_error_message(error_code));
	output_string(property, text);
	return;
    }

    rest_text_length = max_text_size;
    for (;;) {
	/*
	 * Determine how many bytes I should read.
	 */
        if (EB_SIZE_PAGE - 1 < rest_text_length || max_text_size == 0)
            text_length_argument = EB_SIZE_PAGE - 1;
        else
            text_length_argument = rest_text_length;
        if (text_length_argument <= 0)
            break;

	/*
	 * Read text.
	 */
        error_code = eb_read_text(&property->book->book,
	    &property->book->appendix, property->hookset, (VOID *)property,
	    text_length_argument, text, &actual_text_length);
        if (error_code != EB_SUCCESS || actual_text_length == 0)
            break;
        rest_text_length -= actual_text_length;
	*(text + actual_text_length) = '\0';
	output_string(property, text);
    }
}


/*
 * Output glyph of a narrow local character as GIF.
 * (path = /<book-name>/<subbook-name>/narrow.gif)
 */
void
output_narrow_font_character(property)
    Response_Property *property;
{
    char bitmap[EB_SIZE_NARROW_FONT_16];
    char gif[EB_SIZE_NARROW_FONT_16_GIF];
    size_t gif_length;
    EB_Error_Code error_code;

    error_code = eb_narrow_font_character_bitmap(&property->book->book,
	property->local_character, bitmap);
    if (error_code != EB_SUCCESS)
	memset(bitmap, 0x00, EB_SIZE_NARROW_FONT_16);

    eb_bitmap_to_gif(bitmap, EB_WIDTH_NARROW_FONT_16, EB_HEIGHT_FONT_16,
	gif, &gif_length);

    output_data(property, gif, EB_SIZE_NARROW_FONT_16_GIF);
}


/*
 * Output glyph of a wide local character as GIF.
 * (path = /<book-name>/<subbook-name>/wide.gif)
 */
void
output_wide_font_character(property)
    Response_Property *property;
{
    char bitmap[EB_SIZE_WIDE_FONT_16];
    char gif[EB_SIZE_WIDE_FONT_16_GIF];
    size_t gif_length;
    EB_Error_Code error_code;

    error_code = eb_wide_font_character_bitmap(&property->book->book,
	property->local_character, bitmap);
    if (error_code != EB_SUCCESS)
	memset(bitmap, 0x00, EB_SIZE_WIDE_FONT_16);

    eb_bitmap_to_gif(bitmap, EB_WIDTH_WIDE_FONT_16, EB_HEIGHT_FONT_16,
	gif, &gif_length);

    output_data(property, gif, EB_SIZE_WIDE_FONT_16_GIF);
}


/*
 * Output color graphic data.
 * (path = /<book-name>/<subbook-name>/color.bmp
 *      or /<book-name>/<subbook-name>/color.jpg)
 */
void
output_color_graphic(property)
    Response_Property *property;
{
    char binary_data[EB_SIZE_PAGE];
    EB_Error_Code error_code;
    ssize_t read_length;

    error_code = eb_set_binary_color_graphic(&property->book->book, 
	&property->position);
    if (error_code != EB_SUCCESS)
	return;

    for (;;) {
	error_code = eb_read_binary(&property->book->book, EB_SIZE_PAGE,
	    binary_data, &read_length);
	if (error_code != EB_SUCCESS || read_length == 0)
	    return;
	output_data(property, binary_data, read_length);
    }

    /* not reached */
    return;
}


/*
 * Output monochrome (2 colors) graphic data.
 * (path = /<book-name>/<subbook-name>/mono.bmp)
 */
void
output_mono_graphic(property)
    Response_Property *property;
{
    char binary_data[EB_SIZE_PAGE];
    EB_Error_Code error_code;
    ssize_t read_length;

    error_code = eb_set_binary_mono_graphic(&property->book->book, 
	&property->position, property->width, property->height);
    if (error_code != EB_SUCCESS)
	return;

    for (;;) {
	error_code = eb_read_binary(&property->book->book, EB_SIZE_PAGE,
	    binary_data, &read_length);
	if (error_code != EB_SUCCESS || read_length == 0)
	    return;
	output_data(property, binary_data, read_length);
    }

    /* not reached */
    return;
}


/*
 * Output gray scale graphic data.
 * (path = /<book-name>/<subbook-name>/mono.bmp)
 */
void
output_gray_graphic(property)
    Response_Property *property;
{
    char binary_data[EB_SIZE_PAGE];
    EB_Error_Code error_code;
    ssize_t read_length;

    error_code = eb_set_binary_gray_graphic(&property->book->book, 
	&property->position, property->width, property->height);
    if (error_code != EB_SUCCESS)
	return;

    for (;;) {
	error_code = eb_read_binary(&property->book->book, EB_SIZE_PAGE,
	    binary_data, &read_length);
	if (error_code != EB_SUCCESS || read_length == 0)
	    return;
	output_data(property, binary_data, read_length);
    }

    /* not reached */
    return;
}


/*
 * Output WAVE sound data.
 * (path = /<book-name>/<subbook-name>/sound.wav)
 */
void
output_wave(property)
    Response_Property *property;
{
    char binary_data[EB_SIZE_PAGE];
    EB_Error_Code error_code;
    EB_Position end_position;
    ssize_t read_length;

    /*
     * Calcurate positoin where the sound data is terminated.
     */
    end_position.page = property->position.page
	+ (property->size / EB_SIZE_PAGE);
    end_position.offset = property->position.offset
	+ (property->size % EB_SIZE_PAGE);
    if (EB_SIZE_PAGE <= end_position.offset) {
	end_position.offset -= EB_SIZE_PAGE;
	end_position.page++;
    }

    /*
     * Read sound data.
     */
    error_code = eb_set_binary_wave(&property->book->book, 
	&property->position, &end_position);
    if (error_code != EB_SUCCESS)
	return;

    for (;;) {
	error_code = eb_read_binary(&property->book->book, EB_SIZE_PAGE,
	    binary_data, &read_length);
	if (error_code != EB_SUCCESS || read_length == 0)
	    return;
	output_data(property, binary_data, read_length);
    }

    /* not reached */
    return;
}


/*
 * Output MPEG movie data.
 * (path = /<book-name>/<subbook-name>/movie.mpg)
 */
void
output_mpeg(property)
    Response_Property *property;
{
    char binary_data[EB_SIZE_PAGE];
    unsigned int argv[4];
    EB_Error_Code error_code;
    ssize_t read_length;

    if (eb_decompose_movie_file_name(argv, &property->file_name)
	!= EB_SUCCESS)
	return;

    error_code = eb_set_binary_mpeg(&property->book->book, argv);
    if (error_code != EB_SUCCESS)
	return;

    for (;;) {
	error_code = eb_read_binary(&property->book->book, EB_SIZE_PAGE,
	    binary_data, &read_length);
	if (error_code != EB_SUCCESS || read_length == 0)
	    return;
	output_data(property, binary_data, read_length);
    }

    /* not reached */
    return;
}



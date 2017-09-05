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

#include "defs.h"
#include "http.h"
#include "entity.h"

#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif

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
 * Unexported functions.
 */
static EB_Error_Code hook_initialize EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_reference EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_width_height EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_mono_bmp EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_gray_bmp EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_color_bmp EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_color_jpeg EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_wave EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_mpeg EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_candidate_leaf EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_candidate_group EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_newline EBNETD_P((EB_Book *, EB_Appendix *,
    VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_narrow_font_character EBNETD_P((EB_Book *,
    EB_Appendix *, VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_wide_font_character EBNETD_P((EB_Book *,
    EB_Appendix *, VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_begin_superscript EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_end_superscript EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_begin_subscript EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_end_subscript EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_euc_to_ascii EBNETD_P((EB_Book *,
    EB_Appendix *, VOID *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_latin1_to_entity EBNETD_P((EB_Book *,
    EB_Appendix *, VOID *, EB_Hook_Code, int, const unsigned int *));

static void write_text_input_word EBNETD_P((EB_Book *, const char *));

/*
 * Hooks.
 */
static const EB_Hook html_hooks[] = {
    {EB_HOOK_INITIALIZE,	   hook_initialize},
    {EB_HOOK_NEWLINE,              hook_newline},
    {EB_HOOK_NARROW_FONT,          hook_narrow_font_character},
    {EB_HOOK_WIDE_FONT,            hook_wide_font_character},
    {EB_HOOK_END_REFERENCE,        hook_reference},
    {EB_HOOK_BEGIN_MONO_GRAPHIC,   hook_width_height},
    {EB_HOOK_BEGIN_GRAY_GRAPHIC,   hook_width_height},
    {EB_HOOK_END_MONO_GRAPHIC,     hook_mono_bmp},
    {EB_HOOK_END_GRAY_GRAPHIC,     hook_gray_bmp},
    {EB_HOOK_BEGIN_COLOR_BMP,      hook_color_bmp},
    {EB_HOOK_BEGIN_COLOR_JPEG,     hook_color_jpeg},
    {EB_HOOK_BEGIN_WAVE,           hook_wave},
    {EB_HOOK_BEGIN_MPEG,           hook_mpeg},
    {EB_HOOK_END_CANDIDATE_LEAF,   hook_candidate_leaf},
    {EB_HOOK_END_CANDIDATE_GROUP,  hook_candidate_group},
    {EB_HOOK_BEGIN_SUPERSCRIPT,    hook_begin_superscript},
    {EB_HOOK_END_SUPERSCRIPT,      hook_end_superscript},
    {EB_HOOK_BEGIN_SUBSCRIPT,      hook_begin_subscript},
    {EB_HOOK_END_SUBSCRIPT,        hook_end_subscript},
    {EB_HOOK_NARROW_JISX0208,      hook_euc_to_ascii},
    {EB_HOOK_ISO8859_1,            hook_latin1_to_entity},
    {EB_HOOK_NULL,                 NULL}
};

static int graphic_width;
static int graphic_height;

/*
 * Set `html_hooks[]' to `hookset'.
 */
void
set_html_hooks(EB_Hookset *hookset)
{
    eb_set_hooks(hookset, (EB_Hook *)html_hooks);
}


/*
 * Hook for a reference to text.
 */
static int
hook_initialize(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    graphic_width = 0;
    graphic_height = 0;
    return EB_SUCCESS;
}


/*
 * Hook for a reference to text.
 */
static int
hook_reference(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];

    sprintf(text,
	"<a href=\"/%s/%s/text?page=0x%x&offset=0x%x\">(¢ª»²¾È)</a>",
	property->book->name, property->subbook_name, argv[1], argv[2]);
    eb_write_text_string(book, text);
    return EB_SUCCESS;
}


/*
 * Hook for a reference to text.
 */
static int
hook_width_height(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    graphic_width = argv[3];
    graphic_height = argv[2];
    return EB_SUCCESS;
}


/*
 * Hook for a reference to monochrome (2 colors) graphic data.
 */
static int
hook_mono_bmp(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];

    if (argv[1] == 0 || argv[2] == 0)
	return EB_SUCCESS;
    
    sprintf(text,"<a href=\"/%s/%s/mono.bmp",
	property->book->name, property->subbook_name);
    eb_write_text_string(book, text);

    sprintf(text, "?page=0x%x&offset=0x%x&width=%d&height=%d\">(¢ª¿ÞÈÇ)</a>",
	argv[1], argv[2], graphic_width, graphic_height);
    eb_write_text_string(book, text);

    return EB_SUCCESS;
}


/*
 * Hook for a reference to gray scale graphic data.
 */
static int
hook_gray_bmp(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];

    sprintf(text,"<a href=\"/%s/%s/gray.bmp", 
	property->book->name, property->subbook_name);
    eb_write_text_string(book, text);

    sprintf(text, "?page=0x%x&offset=0x%x&width=%d&height=%d\">(¢ª¿ÞÈÇ)</a>",
	argv[1], argv[2], graphic_width, graphic_height);
    eb_write_text_string(book, text);

    return EB_SUCCESS;
}


/*
 * Hook for a reference to BMP graphic data.
 */
static int
hook_color_bmp(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];

    sprintf(text,
	"<a href=\"/%s/%s/color.bmp?page=0x%x&offset=0x%x\">(¢ª¿ÞÈÇ)</a>",
	property->book->name, property->subbook_name, argv[2], argv[3]);
    eb_write_text_string(book, text);
    return EB_SUCCESS;
}


/*
 * Hook for a reference to JPEG graphic data.
 */
static int
hook_color_jpeg(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];

    sprintf(text,
	"<a href=\"/%s/%s/color.jpg?page=0x%x&offset=0x%x\">(¢ª¿ÞÈÇ)</a>",
	property->book->name, property->subbook_name, argv[2], argv[3]);
    eb_write_text_string(book, text);
    return EB_SUCCESS;
}


/*
 * Hook for a reference to WAVE sound data.
 */
static int
hook_wave(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];
    off_t start_location;
    off_t end_location;
    size_t data_size;

    /*
     * Set binary context.
     */
    start_location = (off_t)(argv[2] - 1) * EB_SIZE_PAGE + argv[3];
    end_location   = (off_t)(argv[4] - 1) * EB_SIZE_PAGE + argv[5];
    data_size = end_location - start_location;
    
    sprintf(text, "<a href=\"/%s/%s/sound.wav?page=0x%x&offset=0x%x&size=%d",
	property->book->name, property->subbook_name, argv[2], argv[3],
	(int)data_size);
    eb_write_text_string(book, text);
    eb_write_text_string(book, "\">(¢ª²»À¼)</a>");
    return EB_SUCCESS;
}


/*
 * Hook for a reference to MPEG movie data.
 */
static int
hook_mpeg(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char text[MAX_STRING_LENGTH + 1];

    if (eb_compose_movie_file_name(argv + 2, file_name) != EB_SUCCESS)
	return EB_SUCCESS;
    sprintf(text, "<a href=\"/%s/%s/movie.mpg?filename=%s\">(¢ªÆ°²è)</a>",
	property->book->name, property->subbook_name, file_name);
    eb_write_text_string(book, text);
    return EB_SUCCESS;
}


/*
 * Hook for the current candidate of leaf type.
 */
static int
hook_candidate_leaf(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];
    int i;

    sprintf(text, "<a href=\"/%s/%s/%d/multi?",
	property->book->name, property->subbook_name, property->multi_search);
    eb_write_text_string(book, text);

    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	sprintf(text, "&amp;word%d=", i + 1);
	eb_write_text_string(book, text);
	if (i == property->multi_entry)
	    write_text_input_word(book, eb_current_candidate(book));
	else
	    write_text_input_word(book, property->input_words[i]);
    }

    eb_write_text_string(book, "\">(¢ªÁªÂò)</a>");
    
    return EB_SUCCESS;
}


/*
 * Hook for the current candidate of group type.
 */
static int
hook_candidate_group(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];
    int i;

    if (property->multi_search == EB_MULTI_INVALID)
	return hook_reference(book, appendix, container, code, argc, argv);

    sprintf(text, "<a href=\"/%s/%s/%d/search-multi",
	property->book->name, property->subbook_name, property->multi_search);
    eb_write_text_string(book, text);

    sprintf(text, "?select%d=&amp;page=0x%x&amp;offset=0x%x",
	property->multi_entry + 1, argv[1], argv[2]);
    eb_write_text_string(book, text);

    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	sprintf(text, "&amp;word%d=", i + 1);
	eb_write_text_string(book, text);
	write_text_input_word(book, property->input_words[i]);
    }

    eb_write_text_string(book, "\">(¢ª¾ÜºÙ...)</a>");
    
    return EB_SUCCESS;
}


/*
 * Hook for a reference to text.
 */
static int
hook_newline(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_string(book, "<br>\n");
    return EB_SUCCESS;
}


/*
 * Hook for narrow local character.
 */
EB_Error_Code
hook_narrow_font_character(book, appendix, container, hook_code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code hook_code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];
    char alt_text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];
    unsigned char *alt_p;

    sprintf(text, "<img src=\"/%s/%s/narrow.gif?char=0x%04x\" alt=\"",
        property->book->name, property->subbook_name, argv[0]);
    eb_write_text_string(book, text);

    if (appendix != NULL
	&& eb_narrow_alt_character_text(appendix, argv[0], alt_text)
	== EB_SUCCESS) {
	for (alt_p = (unsigned char *)alt_text; *alt_p != '\0'; alt_p++) {
	    if (*alt_p <= 0x7f && latin1_entity_name_table[*alt_p] != NULL) {
		eb_write_text_byte1(book, '&');
		eb_write_text_string(book, latin1_entity_name_table[*alt_p]);
		eb_write_text_byte1(book, ';');
	    } else {
		eb_write_text_byte1(book, *alt_p);
	    }
	}
    } else {
	eb_write_text_byte1(book, '?');
    }
    eb_write_text_string(book, "\" width=\"8\" height=\"16\">");

    return EB_SUCCESS;
}


/*
 * Hook for wide local character.
 */
EB_Error_Code
hook_wide_font_character(book, appendix, container, hook_code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code hook_code;
    int argc;
    const unsigned int *argv;
{
    Response_Property *property = (Response_Property *)container;
    char text[MAX_STRING_LENGTH + 1];
    char alt_text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];
    unsigned char *alt_p;

    sprintf(text, "<img src=\"/%s/%s/wide.gif?char=0x%04x\" alt=\"",
        property->book->name, property->subbook_name, argv[0]);
    eb_write_text_string(book, text);

    if (appendix != NULL
	&& eb_wide_alt_character_text(appendix, argv[0], alt_text)
	== EB_SUCCESS) {
	for (alt_p = (unsigned char *)alt_text; *alt_p != '\0'; alt_p++) {
	    if (*alt_p <= 0x7f && latin1_entity_name_table[*alt_p] != NULL) {
		eb_write_text_byte1(book, '&');
		eb_write_text_string(book, latin1_entity_name_table[*alt_p]);
		eb_write_text_byte1(book, ';');
	    } else {
		eb_write_text_byte1(book, *alt_p);
	    }
	} 
   } else {
	eb_write_text_byte1(book, '?');
    }
    eb_write_text_string(book, "\" width=\"16\" height=\"16\">");

    return EB_SUCCESS;
}


/*
 * Hook for beginning of superscript.
 */
static int
hook_begin_superscript(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_string(book, "<super>");
    return EB_SUCCESS;
}


/*
 * Hook for end of superscript.
 */
static int
hook_end_superscript(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_string(book, "</super>");
    return EB_SUCCESS;
}


/*
 * Hook for beginning of subscript.
 */
static int
hook_begin_subscript(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_string(book, "<sub>");
    return EB_SUCCESS;
}


/*
 * Hook for end of subscript.
 */
static int
hook_end_subscript(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_string(book, "</sub>");
    return EB_SUCCESS;
}


/*
 * EUC JP to ASCII conversion table.
 */
#define EUC_TO_ASCII_TABLE_START	0xa0
#define EUC_TO_ASCII_TABLE_END		0xff

static const unsigned char euc_a1_to_ascii_table[] = {
    0x00, 0x20, 0x00, 0x00, 0x2c, 0x2e, 0x00, 0x3a,     /* 0xa0 */
    0x3b, 0x3f, 0x21, 0x00, 0x00, 0x00, 0x60, 0x00,     /* 0xa8 */
    0x5e, 0x7e, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xb0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x2f,     /* 0xb8 */
    0x5c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x27,     /* 0xc0 */
    0x00, 0x22, 0x28, 0x29, 0x00, 0x00, 0x5b, 0x5d,     /* 0xc8 */
    0x7b, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xd0 */
    0x00, 0x00, 0x00, 0x00, 0x2b, 0x2d, 0x00, 0x00,     /* 0xd8 */
    0x00, 0x3d, 0x00, 0x3c, 0x3e, 0x00, 0x00, 0x00,     /* 0xe0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c,     /* 0xe8 */
    0x24, 0x00, 0x00, 0x25, 0x23, 0x26, 0x2a, 0x40,     /* 0xf0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xf8 */
};

static const unsigned char euc_a3_to_ascii_table[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xa0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xa8 */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,     /* 0xb0 */
    0x38, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xb8 */
    0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,     /* 0xc0 */
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,     /* 0xc8 */
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,     /* 0xd0 */
    0x58, 0x59, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xd8 */
    0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,     /* 0xe0 */
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,     /* 0xe8 */
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,     /* 0xf0 */
    0x78, 0x79, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00,     /* 0xf8 */
};


/*
 * Hook which converts a character from EUC-JP to ASCII.
 */
EB_Error_Code
hook_euc_to_ascii(book, appendix, container, hook_code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code hook_code;
    int argc;
    const unsigned int *argv;
{
    int in_code1, in_code2;
    int out_code = 0;
    const char *entity;

    in_code1 = argv[0] >> 8;
    in_code2 = argv[0] & 0xff;

    if (in_code2 < EUC_TO_ASCII_TABLE_START
	|| EUC_TO_ASCII_TABLE_END < in_code2) {
	out_code = 0;
    } else if (in_code1 == 0xa1) {
	out_code = euc_a1_to_ascii_table[in_code2 - EUC_TO_ASCII_TABLE_START];
    } else if (in_code1 == 0xa3) {
	out_code = euc_a3_to_ascii_table[in_code2 - EUC_TO_ASCII_TABLE_START];
    }

    if (out_code == 0)
	eb_write_text_byte2(book, in_code1, in_code2);
    else {
	entity = latin1_entity_name_table[out_code];
	if (entity != NULL) {
	    eb_write_text_byte1(book, '&');
	    eb_write_text_string(book, entity);
	    eb_write_text_byte1(book, ';');
	} else {
	    eb_write_text_byte1(book, out_code);
	}
    }
	
    return EB_SUCCESS;
}


/*
 * Hook which converts a Latin-1 character to entity reference.
 * (e.g. 'A  --> &Aacute;
 */
EB_Error_Code
hook_latin1_to_entity(book, appendix, container, hook_code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    VOID *container;
    EB_Hook_Code hook_code;
    int argc;
    const unsigned int *argv;
{
    const char *entity;

    entity = latin1_entity_name_table[argv[0]];
    if (entity != NULL) {
	eb_write_text_byte1(book, '&');
	eb_write_text_string(book, entity);
	eb_write_text_byte1(book, ';');
    } else {
	eb_write_text_byte1(book, argv[0]);
    }

    return EB_SUCCESS;
}


/*
 * Encode an input word with CGI encoding scheme, and write the result
 * to text.
 */
static void
write_text_input_word(book, word)
    EB_Book *book;
    const char *word;
{
    static const char hex_characters[]
	= {'0', '1', '2', '3', '4', '5', '6', '7',
	   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char encoded_word[EB_MAX_WORD_LENGTH + 1];
    size_t encoded_length;
    unsigned char *word_p;
    unsigned char *encoded_p;

    encoded_p = (unsigned char *)encoded_word;
    encoded_length = 0;

    for (word_p = (unsigned char *)word;
	 *word_p != '\0'; word_p++) {

	/*
	 * Send encoded word when `encoded_p' reaches to near of
	 * the end of `encoded_word'.
	 */
	if (EB_MAX_WORD_LENGTH - 3 < encoded_length) {
	    *encoded_p = '\0';
	    eb_write_text_string(book, encoded_word);
	    encoded_p = (unsigned char *)encoded_word;
	    encoded_length = 0;
	}

	/*
	 * Alphabet, digit, '$", '-', '_', and  ',' are simply copied.
	 * ' ' is converted to `+'.
	 * Other characters are converted to "% HEX HEX".
	 */
	if (isalnum(*word_p)
	    || *word_p == '$'
	    || *word_p == '-'
	    || *word_p == '_'
	    || *word_p == '.') {
	    *encoded_p++ = *word_p;
	    encoded_length++;
	} else if (*word_p == ' ') {
	    *encoded_p++ = '+';
	    encoded_length++;
	} else {
	    *encoded_p++ = '%';
	    *encoded_p++ = hex_characters[(*word_p >> 4) & 0x0f];
	    *encoded_p++ = hex_characters[(*word_p)      & 0x0f];
	    encoded_length += 3;
	}
    }

    if (0 < encoded_length) {
	*encoded_p = '\0';
	eb_write_text_string(book, encoded_word);
    }
}



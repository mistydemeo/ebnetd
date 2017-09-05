/*
 * Copyright (c) 1997, 98, 99, 2000, 01  
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

#include "ndtp.h"

/*
 * Unexported functions.
 */
static EB_Error_Code hook_narrow_font_text EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_narrow_font_bitmap EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_wide_font_text EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_wide_font_bitmap EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_reference EBNETD_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_begin_script EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_end_script EBNETD_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
static EB_Error_Code hook_gb2312 EBNETD_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));

/*
 * Hooks.
 */
static const EB_Hook common_hooks[] = {
    {EB_HOOK_NARROW_FONT,         hook_narrow_font_text},
    {EB_HOOK_WIDE_FONT,           hook_wide_font_text},
    {EB_HOOK_END_REFERENCE,       hook_reference},
    {EB_HOOK_END_CANDIDATE_GROUP, hook_reference},
    {EB_HOOK_BEGIN_SUPERSCRIPT,   hook_begin_script},
    {EB_HOOK_BEGIN_SUBSCRIPT,     hook_begin_script},
    {EB_HOOK_END_SUPERSCRIPT,     hook_end_script},
    {EB_HOOK_END_SUBSCRIPT,       hook_end_script},
    {EB_HOOK_GB2312,              hook_gb2312},
    {EB_HOOK_NULL,                NULL}
};

static const EB_Hook text_hooks[] = {
    {EB_HOOK_NARROW_FONT,         hook_narrow_font_text},
    {EB_HOOK_WIDE_FONT,           hook_wide_font_text},
    {EB_HOOK_NULL,                NULL}
};

static const EB_Hook bitmap_hooks[] = {
    {EB_HOOK_NARROW_FONT,         hook_narrow_font_bitmap},
    {EB_HOOK_WIDE_FONT,           hook_wide_font_bitmap},
    {EB_HOOK_NULL,                NULL}
};


/*
 * Set `common_hooks[]' to `hookset'.
 */
void
set_common_hooks(EB_Hookset *hookset)
{
    eb_set_hooks(hookset, (EB_Hook *)common_hooks);
}


/*
 * Set `bitmap_hooks[]' to `hookset'.
 */
void
set_bitmap_hooks(EB_Hookset *hookset)
{
    eb_set_hooks(hookset, (EB_Hook *)bitmap_hooks);
}


/*
 * Set `text_hooks[]' to `hookset'.
 */
void
set_text_hooks(EB_Hookset *hookset)
{
    eb_set_hooks(hookset, (EB_Hook *)text_hooks);
}


/*
 * Hook for narrow font characters.
 */
static int
hook_narrow_font_text(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char alt_text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];

    if (eb_narrow_alt_character_text(appendix, argv[0], alt_text)
	!= EB_SUCCESS)
	sprintf(alt_text, "#%04x", argv[0]);
    eb_write_text_string(book, alt_text);

    return EB_SUCCESS;
}


/*
 * Hook for narrow font characters.
 */
static int
hook_narrow_font_bitmap(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char tag[NDTP_MAX_TAG_LENGTH + 1];

    sprintf(tag, "<gaiji:h%04x>", argv[0]);
    eb_write_text_string(book, tag);

    return EB_SUCCESS;
}


/*
 * Hook for wide font characters.
 */
static int
hook_wide_font_text(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char alt_text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];

    if (eb_wide_alt_character_text(appendix, argv[0], alt_text)
	!= EB_SUCCESS)
	sprintf(alt_text, "#%04x", argv[0]);
    eb_write_text_string(book, alt_text);

    return EB_SUCCESS;
}


/*
 * Hook for wide font characters.
 */
static int
hook_wide_font_bitmap(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char tag[NDTP_MAX_TAG_LENGTH + 1];

    sprintf(tag, "<gaiji:z%04x>", argv[0]);
    eb_write_text_string(book, tag);

    return EB_SUCCESS;
}


/*
 * Hook for a reference to text.
 */
static int
hook_reference(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char tag[NDTP_MAX_TAG_LENGTH + 1];

    sprintf(tag, "<%x:%x>", argv[1], argv[2]);
    eb_write_text_string(book, tag);
    
    return EB_SUCCESS;
}


/*
 * Hook for beginning of superscript or subscript.
 */
static int
hook_begin_script(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_byte1(book, '(');
    return EB_SUCCESS;
}


/*
 * Hook for end of superscript or subscript.
 */
static int
hook_end_script(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    eb_write_text_byte1(book, ')');
    return EB_SUCCESS;
}


/*
 * Hook for a GB 2312 character.
 */
static int
hook_gb2312(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char tag[NDTP_MAX_TAG_LENGTH + 1];
    unsigned char c1, c2;

    c1 = (argv[0] >> 8) & 0xff;
    c2 =  argv[0]       & 0xff;
    sprintf(tag, "<&g0:%02d%02d>", c1 - 0xa0, c2 - 0xa0);
    eb_write_text_string(book, tag);

    return EB_SUCCESS;
}



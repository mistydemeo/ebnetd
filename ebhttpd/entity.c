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

/*
 * Latin-1 character to entity reference table.
 * (e.g. 'a  --> &aacute;)
 */
const char *latin1_entity_name_table[] = {
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x00 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x08 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x10 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x18 */
    NULL, NULL, "quot", NULL, NULL, NULL, "amp", NULL,	/* 0x20 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x28 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x30 */
    NULL, NULL, NULL,   NULL, "lt", NULL, "gt",  NULL,	/* 0x38 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x40 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x48 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x50 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x58 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x60 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x68 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x70 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x78 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x80 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x88 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x90 */
    NULL, NULL, NULL,   NULL, NULL, NULL, NULL,  NULL,	/* 0x98 */
    "nbsp",   "iexcl",  "cent",   "pound",	/* 0xa0 */
    "curren", "yen",    "brvbar", "sect",	/* 0xa4 */
    "uml",    "copy",   "ordf",   "laquo",	/* 0xa8 */
    "not",    "shy",    "reg",    "macr",	/* 0xac */
    "deg",    "plusmn", "sup2",   "sup3",	/* 0xb0 */
    "acute",  "micro",  "para",   "middot",	/* 0xb4 */
    "cedil",  "sup1",   "ordm",   "requo",	/* 0xb8 */
    "frac14", "frac12", "farc34", "iquest",	/* 0xbc */
    "Agrave", "Aacute", "Acirc",  "Atilde",	/* 0xc0 */
    "Auml",   "Aring",  "AElig",  "Ccedil",	/* 0xc4 */
    "Egrave", "Eacute", "Ecirc",  "Euml",	/* 0xc8 */
    "Igrave", "Iacute", "Icirc",  "Iuml",	/* 0xcc */
    "ETH",    "Ntilde", "Ograve", "Oacute",	/* 0xd0 */
    "Ocirc",  "Otilde", "Ouml",   "times",	/* 0xd4 */
    "Oslash", "Ugrave", "Uacute", "Ucirc",	/* 0xd8 */
    "Uuml",   "Yacute", "THORN",  "szlig",	/* 0xdc */
    "agrave", "aacute", "acirc",  "atilde",	/* 0xe0 */
    "auml",   "aring",  "aelig",  "ccedil",	/* 0xe4 */
    "egrave", "eacute", "ecirc",  "euml",	/* 0xe8 */
    "igrave", "iacute", "icirc",  "iuml",	/* 0xec */
    "eth",    "ntilde", "ograve", "oacute",	/* 0xf0 */
    "ocirc",  "otilde", "ouml",   "divide",	/* 0xf4 */
    "oslash", "ugrave", "uacute", "ucirc",	/* 0xf8 */
    "uuml",   "yacute", "thorn",  "yuml"	/* 0xfc */
};

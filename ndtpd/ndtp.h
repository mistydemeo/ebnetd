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

#ifndef NDTP_H
#define NDTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <eb/eb.h>
#include <eb/error.h>
#include <eb/appendix.h>
#include <eb/font.h>
#include <eb/text.h>

#include "defs.h"

/*
 * Maximum length definitions for NDTP.
 */
#define NDTP_MAX_LINE_LENGTH 		511

/*
 * Length of short buffer.
 */
#define NDTP_SHORT_BUFFER_LENGTH 	15

/*
 * Maximum length of a tag.
 */
#define NDTP_MAX_TAG_LENGTH		31

/*
 * Function declarations.
 */
void set_common_hooks EBNETD_P((EB_Hookset *));
void set_bitmap_hooks EBNETD_P((EB_Hookset *));
void set_text_hooks EBNETD_P((EB_Hookset *));

#endif /* NDTP_H */

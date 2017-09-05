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

#ifndef ENTITY_H
#define ENTITY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Max length of an HTML entity name (e.g. "&divide;").
 * Note that the name includes `&' and `;'.
 */
#define MAX_ENTITY_NAME_LENGTH	8

/*
 * Global variables.
 */
extern const char *latin1_entity_name_table[];

#endif /* ENTITY_H */

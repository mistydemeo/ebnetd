/*
 * Copyright (c) 1997, 2000, 01  
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

#ifndef OPENMAX_H
#define OPENMAX_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/*
 * Function declarations.
 */
#ifdef __STDC__
int get_open_max(void);
#else
int get_open_max();
#endif

#endif /* not OPENMAX_H */

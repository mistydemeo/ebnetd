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

#ifndef SIGNAME_H
#define SIGNAME_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Function declarations.
 */
#ifdef __STDC__
int signal_code(const char *);
const char *signal_name(int);
const char *signal_explanation(int);
#ifndef HAVE_PSIGNAL
void psignal(int, const char *);
#endif
#ifndef HAVE_STRSIGNAL
const char *strsignal(int);
#endif

#else /* not __STDC__ */

int signal_code();
const char *signal_name();
const char *signal_explanation();
#ifndef HAVE_PSIGNAL
void psignal();
#endif
#ifndef HAVE_STRSIGNAL
const char *strsignal();
#endif
#endif /* not __STDC__ */

#endif /* not SIGNAME_H */

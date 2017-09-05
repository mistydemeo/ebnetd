/*
 * Copyright (c) 1997, 98, 99, 2000, 01, 02
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

#include "defs.h"

/*
 * Output the version number to standard out.
 */
void
output_version()
{
    printf("%s (EBNETD) version %s\n", program_name, program_version);
    printf("Copyright (c) 1997 - 2003\n");
    printf("   Motoyuki Kasahara\n");
    fflush(stdout);
}


/*
 * Output ``try ...'' message to standard error.
 */
void
output_try_help()
{
    fprintf(stderr, "try `%s --help' for more information\n", invoked_name);
    fflush(stderr);
}


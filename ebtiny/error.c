/*
 * Copyright (c) 2003
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

#include "ebtiny/eb.h"
#include "ebtiny/error.h"


/*
 * Error messages.
 */
static const char * const error_messages[] = {
    /* 0 -- 4 */
    "no error",
    "an empty file name",
    "too long file name",
    "bad file name",
    "bad directory name",

    /* 5 -- 10 */
    "failed to open a catalog file in the book/appendix",
    "failed to read a catalog file in the book/appendix",
    "unexpected format in a catalog file in the book/appendix",
    "book/appendix not bound",
    "no such subbook in the book/appendix",

    NULL
};

/*
 * Look up the error message corresponding to the error code `error_code'.
 */
const char *
eb_error_message(error_code)
    EB_Error_Code error_code;
{
    const char *message;

    if (0 <= error_code && error_code < EB_NUMBER_OF_ERRORS)
        message = error_messages[error_code];
    else
        message = "unknown error";

    return message;
}

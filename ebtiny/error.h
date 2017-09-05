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

#ifndef TINYEB_ERROR_H
#define TINYEB_ERROR_H

#include "ebtiny/eb.h"

/*
 * Error codes.
 */
#define EB_SUCCESS			0
#define EB_ERR_EMPTY_FILE_NAME		1
#define EB_ERR_TOO_LONG_FILE_NAME	2
#define EB_ERR_BAD_FILE_NAME		3
#define EB_ERR_BAD_DIR_NAME		4
#define EB_ERR_FAIL_OPEN_CAT		5
#define EB_ERR_FAIL_READ_CAT		6
#define EB_ERR_UNEXP_CAT		7
#define EB_ERR_UNBOUND_BOOK		8
#define EB_ERR_NO_SUCH_SUB		9

/*
 * The number of error codes.
 */
#define EB_NUMBER_OF_ERRORS             10


/*
 * Function declarations.
 */
const char *eb_error_message EB_P((EB_Error_Code));

#endif /* not TINYEB_ERROR_H */

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

#ifndef TINYEB_EB_H
#define TINYEB_EB_H

#include <sys/types.h>

/*
 * Special subbook code for representing error state.
 */
#define EB_SUBBOOK_INVALID              -1

/*
 * Maximum length of a directory name.
 */
#define EB_MAX_DIRECTORY_NAME_LENGTH    8

/*
 * Maximum length of a file name under a certain directory.
 * prefix(8 chars) + '.' + suffix(3 chars) + ';' + digit(1 char)
 */
#define EB_MAX_FILE_NAME_LENGTH         14

/*
 * Maximum number of subbooks in a book.
 */
#define EB_MAX_SUBBOOKS                 50

/*
 * Trick for function prototypes.
 */
#ifndef EB_P
#if defined(__STDC__) || defined(__cplusplus) || defined(WIN32)
#define EB_P(p) p
#else /* not (__STDC__ && __cplusplus && WIN32) */
#define EB_P(p) ()
#endif /* not (__STDC__ && __cplusplus && WIN32) */
#endif /* EB_P */

/*
 * Types for various codes.
 */
typedef int EB_Error_Code;
typedef int EB_Subbook_Code;

/*
 * Typedef for Structures.
 */
typedef struct EB_Book_Struct	EB_Book;
typedef struct EB_Book_Struct	EB_Appendix;

/*
 * Book
 */
struct EB_Book_Struct {
    int subbook_count;
    char subbooks[EB_MAX_SUBBOOKS][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
};


/*
 * Function declarations.
 */
EB_Error_Code eb_initialize_library EB_P((void));
void eb_finalize_library EB_P((void));
void eb_initialize_book EB_P((EB_Book *));
EB_Error_Code eb_bind EB_P((EB_Book *, const char *));
void eb_finalize_book EB_P((EB_Book *));
EB_Error_Code eb_subbook_list EB_P((EB_Book *, EB_Subbook_Code *, int *));
EB_Error_Code eb_subbook_directory2 EB_P((EB_Book *, EB_Subbook_Code,
    char *));
EB_Error_Code eb_load_all_subbooks EB_P((EB_Book *));
EB_Error_Code eb_fix_directory_name EB_P((const char *, char *));
EB_Error_Code eb_find_file_name EB_P((const char *, const char *, char *));
ssize_t eb_read_all EB_P((int, char *, size_t));

#endif /* not TINYEB_EB_H */

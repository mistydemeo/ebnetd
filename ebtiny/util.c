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
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* not HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* not HAVE_DIRENT_H */

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')

#include "ebtiny/eb.h"
#include "ebtiny/error.h"


/*
 * Rewrite `directory_name' to a real directory name in the `path' directory.
 * 
 * If a directory matched to `directory_name' exists, then EB_SUCCESS is
 * returned, and `directory_name' is rewritten to that name.  Otherwise
 * EB_ERR_BAD_DIR_NAME is returned.
 */
EB_Error_Code
eb_fix_directory_name(path, directory_name)
    const char *path;
    char *directory_name;
{
    struct dirent *entry;
    DIR *dir;

    /*
     * Open the directory `path'.
     */
    dir = opendir(path);
    if (dir == NULL)
        goto failed;

    for (;;) {
        /*
         * Read the directory entry.
         */
        entry = readdir(dir);
        if (entry == NULL)
            goto failed;

	if (strcasecmp(entry->d_name, directory_name) == 0)
	    break;
    }

    strcpy(directory_name, entry->d_name);
    closedir(dir);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (dir != NULL)
	closedir(dir);
    return EB_ERR_BAD_DIR_NAME;
}


#define FOUND_NONE		0
#define FOUND_EBZ		1
#define FOUND_BASENAME		2
#define FOUND_ORG		3

/*
 * Rewrite `found_file_name' to a real file name in the `path_name'
 * directory.
 * 
 * If a file matched to `rarget_file_name' exists, then EB_SUCCESS
 * is returned, and `found_file_name' is rewritten to that name.
 * Otherwise EB_ERR_BAD_FILE_NAME is returned.
 *
 * Note that `target_file_name' must not contain `.' or excceed
 * EB_MAX_DIRECTORY_NAME_LENGTH characters.
 */
EB_Error_Code
eb_find_file_name(path_name, target_file_name, found_file_name)
    const char *path_name;
    const char *target_file_name;
    char *found_file_name;
{
    char ebz_target_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char org_target_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char candidate_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    DIR *dir;
    struct dirent *entry;
    size_t d_namlen;
    int found = FOUND_NONE;

    strcpy(ebz_target_file_name, target_file_name);
    strcat(ebz_target_file_name, ".ebz");
    strcpy(org_target_file_name, target_file_name);
    strcat(org_target_file_name, ".org");
    candidate_file_name[0] = '\0';

    /*
     * Open the directory `path_name'.
     */
    dir = opendir(path_name);
    if (dir == NULL)
	goto failed;

    for (;;) {
	/*
	 * Read the directory entry.
	 */
	entry = readdir(dir);
	if (entry == NULL)
	    break;

	/*
	 * Compare the given file names and the current entry name.
	 * We consider they are matched when one of the followings
	 * is true:
	 *
	 *   <target name>          == <entry name>
	 *   <target name>+";1'     == <entry name>
	 *   <target name>+"."      == <entry name>
	 *   <target name>+".;1"    == <entry name>
	 *   <target name>+".ebz"   == <entry name>
	 *   <target name>+".ebz;1" == <entry name>
	 *   <target name>+".org"   == <entry name>
	 *   <target name>+".org;1" == <entry name>
	 *
	 * All the comparisons are done without case sensitivity.
	 * We support version number ";1" only.
	 */
	d_namlen = NAMLEN(entry);
	if (2 < d_namlen
	    && *(entry->d_name + d_namlen - 2) == ';'
	    && isdigit(*(entry->d_name + d_namlen - 1))) {
	    d_namlen -= 2;
	}
	if (1 < d_namlen && *(entry->d_name + d_namlen - 1) == '.')
	    d_namlen--;

	if (strcasecmp(entry->d_name, ebz_target_file_name) == 0
	    && *(ebz_target_file_name + d_namlen) == '\0'
	    && found < FOUND_EBZ) {
	    strcpy(candidate_file_name, entry->d_name);
	    found = FOUND_EBZ;
	}
	if (strncasecmp(entry->d_name, target_file_name, d_namlen) == 0
	    && *(target_file_name + d_namlen) == '\0'
	    && found < FOUND_BASENAME) {
	    strcpy(candidate_file_name, entry->d_name);
	    found = FOUND_BASENAME;
	}
	if (strcasecmp(entry->d_name, org_target_file_name) == 0
	    && *(org_target_file_name + d_namlen) == '\0'
	    && found < FOUND_ORG) {
	    strcpy(candidate_file_name, entry->d_name);
	    found = FOUND_ORG;
	    break;
	}
    }

    if (found == FOUND_NONE)
	goto failed;

    closedir(dir);
    strcpy(found_file_name, candidate_file_name);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (dir != NULL)
	closedir(dir);
    return EB_ERR_BAD_FILE_NAME;
}


ssize_t
eb_read_all(file, buffer, length)
    int file;
    char *buffer;
    size_t length;
{
    char *buffer_p = buffer;
    ssize_t rest_length = length;
    ssize_t n;

    while (0 < rest_length) {
        errno = 0;
        n = read(file, buffer_p, (size_t)rest_length);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        } else if (n == 0)
            break;
        else {
            rest_length -= n;
            buffer_p += n;
        }
    }

    return length - rest_length;
}

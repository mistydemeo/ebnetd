/*
 * Copyright (c) 1997, 98, 2000, 01  
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

/*
 * This program requires the following Autoconf macros:
 *   AC_TYPE_UID_T
 *   AC_TYPE_GID_T
 *   AC_CHECK_HEADERS(unistd.h)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif


/*
 * Set UID and GID of the current process.
 * To set a UID, the current UID of the process must be root.
 *
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
int
set_privilege(new_uid, new_gid)
    uid_t new_uid;
    gid_t new_gid;
{
    struct passwd *password_entry;
    uid_t current_uid;
    gid_t current_gid;
    int gid_ok = 0;

    /*
     * Set a GID.
     */
    current_gid = getgid();
    if (new_gid != current_gid) {
	password_entry = getpwuid(new_uid);
	if (password_entry == NULL) {
	    syslog(LOG_ERR, "getpwuid() failed, %s: uid=%d", strerror(errno),
		(int)new_uid);
	    goto failed;
	}
	if (initgroups(password_entry->pw_name, new_gid) < 0) {
	    syslog(LOG_ERR, "initgroups() failed, %s: gid=%d", strerror(errno),
		(int)new_gid);
	    goto failed;
	}
	if (setgid(new_gid) < 0) {
	    syslog(LOG_ERR, "setgid() fialed, %s: gid=%d", strerror(errno),
		(int)new_gid);
	    goto failed;
	}
    }
    gid_ok = 1;

    /*
     * Set an UID only when the UID of the current process is root.
     */
    current_uid = getuid();
    if (current_uid != new_uid) {
	if (current_uid != 0)
	    goto failed;
	if (setuid(new_uid) < 0) {
	    syslog(LOG_ERR, "setuid() fialed, %s: uid=%d", strerror(errno),
		(int)new_uid);
	    goto failed;
	}
    }

    syslog(LOG_DEBUG, "debug: set privilege: uid=%d, gid=%d",
	new_uid, new_gid);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (gid_ok)
	syslog(LOG_ERR, "failed to set owner of the process");
    else
	syslog(LOG_ERR, "failed to set owner and group of the process");
    return -1;
}

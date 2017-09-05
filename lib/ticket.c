/*                                                            -*- C -*-
 * Copyright (c) 1999, 2000, 01  
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
 *   AC_C_CONST
 *   AC_CHECK_HEADERS(stdlib.h, unistd.h, fcntl.h)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
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

#include "ticket.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * Whence parameter for lseek().
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif

/*
 * initialize a stock file.
 */
void
initialize_ticket_stock(stock)
    Ticket_Stock *stock;
{
    stock->file_name = NULL;
    stock->file = -1;
    stock->count = -1;
    stock->lock_spot = -1;
}


/*
 * Bind a ticket stock to a lock file `file_name', and set the number
 * of tickets on the stock as `lockmax'.
 *
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
bind_ticket_stock(stock, file_name, lock_max)
    Ticket_Stock *stock;
    const char *file_name;
    int lock_max;
{
    /*
     * Record the lock file_name to the stock.
     */
    stock->file_name = (char *) malloc(strlen(file_name) + 1);
    if (stock->file_name == NULL) {
	syslog(LOG_ERR, "memory exhausted");
	goto failed;
    }
    strcpy(stock->file_name, file_name);

    /*
     * Open the lock file `file_name'.
     */
    stock->file = open(file_name, O_RDWR | O_CREAT | O_BINARY, 0600);
    if (stock->file < 0) {
	syslog(LOG_ERR, "failed to open the file, %s: %s", strerror(errno),
	    file_name);
	goto failed;
    }
    stock->lock_spot = -1;
    stock->count = lock_max;

    syslog(LOG_DEBUG, "debug: open the lock file: %s", file_name);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    finalize_ticket_stock(stock);
    return -1;
}


/*
 * Get a ticket from `stock'.
 * If it succeeds, or we have gotten a ticket from the stock, then
 * 0 is returned.  Otherwise -1 is returned.
 */
int
get_ticket(stock)
    Ticket_Stock *stock;
{
    struct flock lock;
    int i;

    if (stock->file < 0)
	return -1;

    if (0 <= stock->lock_spot)
	return 0;

    /*
     * Try to lock a free spot in the lock file.
     */
    for (i = 0; i < stock->count; i++) {
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = i;
        lock.l_len = 1;
        lock.l_pid = 0;
	if (fcntl(stock->file, F_SETLK, &lock) == 0) {
	    syslog(LOG_DEBUG, "debug: lock the spot %d on the lock file: %s",
		i, stock->file_name);
	    stock->lock_spot = i;
	    return 0;
	}
    }

    syslog(LOG_DEBUG, "debug: no free spot is left on the lock file: %s",
	stock->file_name);
    return -1;
}


/*
 * Release a ticket that is gotten from `stock' beforehand.
 * 
 * If it succeeds to release or no lock is set, then 0 is returned.
 * Otherwise -1 is returned.
 */
int
unget_ticket(stock)
    Ticket_Stock *stock;
{
    struct flock lock;

    if (stock->file < 0 || stock->lock_spot < 0)
	return 0;

    /*
     * Unlock the spot.
     */
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = stock->lock_spot;
    lock.l_len = 1;
    lock.l_pid = 0;
    if (fcntl(stock->file, F_SETLK, &lock) != 0) {
	syslog(LOG_ERR, "failed to unlock the spot %d on the file, %s: %s",
	    stock->lock_spot, strerror(errno), stock->file_name);
	return -1;
    }
    stock->lock_spot = -1;

    return 0;
}


/*
 * Test whether `stock' has had a ticket or not.
 */
int
have_ticket(stock)
    Ticket_Stock *stock;
{
    return (0 <= stock->file && 0 <= stock->lock_spot);
}


/*
 * Clear the ticket stock.
 */
void
finalize_ticket_stock(stock)
    Ticket_Stock *stock;
{
    /*
     * Close the lock file.
     */
    if (0 <= stock->file) {
	close(stock->file);
	syslog(LOG_DEBUG, "debug: close the lock file: %s", stock->file_name);
	stock->file = -1;
    }

    /*
     * Dispose memory allocated to the file_name.
     */
    if (stock->file_name != NULL) {
	free(stock->file_name);
	stock->file_name = NULL;
    }

    stock->count = -1;
    stock->lock_spot = -1;
}

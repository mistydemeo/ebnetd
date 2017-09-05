/*
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

#ifndef TICKET_H
#define TICKET_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/*
 * Ticket Stock.
 */
typedef struct {
    char *file_name;		/* lock file name */
    int file;			/* lock file descriptor */
    int count;			/* the number of tickets */
    int lock_spot;		/* locked spot (gotten ticket) */
} Ticket_Stock;


/*
 * Function declarations.
 */
#ifdef __STDC__
void initialize_ticket_stock(Ticket_Stock *);
int bind_ticket_stock(Ticket_Stock *, const char *, int);
int get_ticket(Ticket_Stock *);
int unget_ticket(Ticket_Stock *);
int have_ticket(Ticket_Stock *);
void finalize_ticket_stock(Ticket_Stock *);

#else /* not __STDC__ */
void initialize_ticket_stock();
int bind_ticket_stock();
int get_ticket();
int unget_ticket();
int have_ticket();
void finalize_ticket_stock();
#endif /* not __STDC__ */

#endif /* not TICKET_H */

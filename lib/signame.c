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
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, unistd.h)
 *   AC_CHECK_FUNCS(strcasecmp, psignal, strsignal)
 *   AC_DECL_SYS_SIGLIST
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

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

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp();
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

#ifndef NSIG
#ifdef  _NSIG
#define NSIG    _NSIG
#else
#define NSIG    32
#endif
#endif


/*
 * Signal codes, names, and explanations.
 * The table is taken from `signame.c' in GNU make version 3.75.
 */
/* Convert between signal names and numbers.
   Copyright (C) 1990, 1992, 1993, 1995, 1996 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

typedef struct {
    int code;
    const char *name;
    const char *explanation;
} Signal_Entry;

static const Signal_Entry signal_table[] = {
    /* Initialize signal names.  */
#ifdef SIGHUP
    {SIGHUP, "HUP", "Hangup"},
#endif
#ifdef SIGINT
    {SIGINT, "INT", "Interrupt"},
#endif
#ifdef SIGQUIT
    {SIGQUIT, "QUIT", "Quit"},
#endif
#ifdef SIGILL
    {SIGILL, "ILL", "Illegal Instruction"},
#endif
#ifdef SIGTRAP
    {SIGTRAP, "TRAP", "Trace/breakpoint trap"},
#endif
    /*
     * If SIGIOT == SIGABRT, we want to print it as SIGABRT because
     * SIGABRT is in ANSI and POSIX.1 and SIGIOT isn't.
     */
#ifdef SIGABRT
    {SIGABRT, "ABRT", "Aborted"},
#endif
#ifdef SIGIOT
    {SIGIOT, "IOT", "IOT trap"},
#endif
#ifdef SIGEMT
    {SIGEMT, "EMT", "EMT trap"},
#endif
#ifdef SIGFPE
    {SIGFPE, "FPE", "Floating point exception"},
#endif
#ifdef SIGKILL
    {SIGKILL, "KILL", "Killed"},
#endif
#ifdef SIGBUS
    {SIGBUS, "BUS", "Bus error"},
#endif
#ifdef SIGSEGV
    {SIGSEGV, "SEGV", "Segmentation fault"},
#endif
#ifdef SIGSYS
    {SIGSYS, "SYS", "Bad system call"},
#endif
#ifdef SIGPIPE
    {SIGPIPE, "PIPE", "Broken pipe"},
#endif
#ifdef SIGALRM
    {SIGALRM, "ALRM", "Alarm clock"},
#endif
#ifdef SIGTERM
    {SIGTERM, "TERM", "Terminated"},
#endif
#ifdef SIGUSR1
    {SIGUSR1, "USR1", "User defined signal 1"},
#endif
#ifdef SIGUSR2
    {SIGUSR2, "USR2", "User defined signal 2"},
#endif
    /*
     * If SIGCLD == SIGCHLD, we want to print it as SIGCHLD because that
     * is what is in POSIX.1.
     */
#ifdef SIGCHLD
    {SIGCHLD, "CHLD", "Child exited"},
#endif
#ifdef SIGCLD
    {SIGCLD, "CLD", "Child exited"},
#endif
#ifdef SIGPWR
    {SIGPWR, "PWR", "Power failure"},
#endif
#ifdef SIGTSTP
    {SIGTSTP, "TSTP", "Stopped"},
#endif
#ifdef SIGTTIN
    {SIGTTIN, "TTIN", "Stopped (tty input)"},
#endif
#ifdef SIGTTOU
    {SIGTTOU, "TTOU", "Stopped (tty output)"},
#endif
#ifdef SIGSTOP
    {SIGSTOP, "STOP", "Stopped (signal)"},
#endif
#ifdef SIGXCPU
    {SIGXCPU, "XCPU", "CPU time limit exceeded"},
#endif
#ifdef SIGXFSZ
    {SIGXFSZ, "XFSZ", "File size limit exceeded"},
#endif
#ifdef SIGVTALRM
    {SIGVTALRM, "VTALRM", "Virtual timer expired"},
#endif
#ifdef SIGPROF
    {SIGPROF, "PROF", "Profiling timer expired"},
#endif
  /*
   * "Window size changed" might be more accurate, but even if that
   * is all that it means now, perhaps in the future it will be
   * extended to cover other kinds of window changes.
   */
#ifdef SIGWINCH
    {SIGWINCH, "WINCH", "Window changed"},
#endif
#ifdef SIGCONT
    {SIGCONT, "CONT", "Continued"},
#endif
#ifdef SIGURG
    {SIGURG, "URG", "Urgent I/O condition"},
#endif
    /* "I/O pending" has also been suggested.  A disadvantage is
     * that signal only happens when the process has
     * asked for it, not everytime I/O is pending.  Another disadvantage
     * is the confusion from giving it a different name than under Unix.
     */
#ifdef SIGIO
    {SIGIO, "IO", "I/O possible"},
#endif
#ifdef SIGWIND
    {SIGWIND, "WIND", "SIGWIND"},
#endif
#ifdef SIGPHONE
    {SIGPHONE, "PHONE", "SIGPHONE"},
#endif
#ifdef SIGPOLL
    {SIGPOLL, "POLL", "I/O possible"},
#endif
#ifdef SIGLOST
    {SIGLOST, "LOST", "Resource lost"},
#endif
#ifdef SIGDANGER
    {SIGDANGER, "DANGER", "Danger signal"},
#endif
#ifdef SIGINFO
    {SIGINFO, "INFO", "Information request"},
#endif
    {-1, NULL, NULL}
};


/*
 * Return the abbreviation name for signal number `code'.
 */
const char *
signal_name(code)
    int code;
{
    const Signal_Entry *entry;

    for (entry = signal_table; entry->name != NULL; entry++) {
	if (entry->code == code)
	    return entry->name;
    }

    return NULL;
}


/*
 * Return the signal number for an abbreviated signal name `name',
 * or -1 if there is no signal by that name.
 */
int
signal_code(name)
    const char *name;
{
    const Signal_Entry *entry;

    for (entry = signal_table; entry->name != NULL; entry++) {
	if (strcasecmp(entry->name, name) == 0)
	    return entry->code;
    }

    return -1;
}


/*
 * Return the signal explanation for an signal `code', or NULL if there
 * is no signal `code'.
 */
const char *
signal_explanation(code)
    int code;
{
    const Signal_Entry *entry;
    
    for (entry = signal_table; entry->name != NULL; entry++) {
	if (entry->code == code)
	    return entry->explanation;
    }

    return NULL;
}


/*
 * Print the name of SIGNAL and message to standard error.
 */
#ifndef HAVE_PSIGNAL
void
psignal(code, message)
    int code;
    const char *message;
{
    const Signal_Entry *entry;
    const char *default_message = NULL;
    
    if (message != NULL)
        fprintf(stderr, "%s: ", message);

#ifdef SYS_SIGLIST_DECLARED
    if (0 < code && code < NSIG)
	default_message = (const char *) sys_siglist[code];

#else /* not SYS_SIGLIST_DECLARED */
    for (entry = signal_table; entry->name != NULL; entry++) {
	if (entry->code == code) {
	    default_message = entry->explanation;
	    break;
	}
    }
#endif /* not SYS_SIGLIST_DECLARED */

    if (default_message == NULL)
	fprintf(stderr, "unknown signal\n");
    else
	fprintf(stderr, "%s\n", default_message);

}
#endif /* not HAVE_PSIGNAL */


/*
 * Return the string associated with the signal number.
 */
#ifndef HAVE_STRSIGNAL
const char *
strsignal(code)
    int code;
{
#ifndef SYS_SIGLIST_DECLARED
    const Signal_Entry *entry;

    for (entry = signal_table; entry->name != NULL; entry++) {
	if (entry->code == code)
	    return entry->explanation;
    }
#else /* not SYS_SIGLIST_DECLARED */
    if (0 < code && code < NSIG)
	return (const char *) sys_siglist[code];
#endif /* not SYS_SIGLIST_DECLARED */

    return "Unknown signal";
}
#endif /* not HAVE_STRSIGNAL */

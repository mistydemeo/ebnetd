dnl * 
dnl * Check for union wait (taken from GNU make 3.75).
dnl *
AC_DEFUN(AC_UNION_WAIT,
[AC_CHECK_FUNCS(waitpid)
AC_CACHE_CHECK(for union wait, ac_cv_union_wait,
[AC_TRY_LINK([
#include <sys/types.h>
#include <sys/wait.h>],[
union wait status;
int pid;
pid = wait (&status);
#ifdef WEXITSTATUS
/* Some POSIXoid systems have both the new-style macros and the old
   union wait type, and they do not work together.  If union wait
   conflicts with WEXITSTATUS et al, we don't want to use it at all.  */
if (WEXITSTATUS (status) != 0) pid = -1;
#ifdef WTERMSIG
/* If we have WEXITSTATUS and WTERMSIG, just use them on ints.  */
-- blow chunks here --
#endif
#endif
#ifdef HAVE_WAITPID
/* Make sure union wait works with waitpid.  */
pid = waitpid (-1, &status, 0);
#endif
], [ac_cv_union_wait=yes], [ac_cv_union_wait=no])])
if test "$ac_cv_union_wait" = yes; then
  AC_DEFINE(HAVE_UNION_WAIT, 1,
[Define this if you have the \`union wait' type in <sys/wait.h>.])
fi
])

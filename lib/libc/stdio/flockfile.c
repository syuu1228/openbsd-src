/*	$OpenBSD: src/lib/libc/stdio/flockfile.c,v 1.3 2001/08/30 07:38:27 fgsch Exp $	*/

#include <stdio.h>
#include "thread_private.h"

/*
 * Subroutine versions of the macros in <stdio.h>
 * Note that these are all no-ops because libc does not do threads.
 * Strong implementation of file locking in libc_r/uthread/uthread_file.c
 */

#undef flockfile
#undef ftrylockfile
#undef funlockfile
#undef _flockfile_debug

WEAK_PROTOTYPE(flockfile);
WEAK_PROTOTYPE(ftrylockfile);
WEAK_PROTOTYPE(funlockfile);
WEAK_PROTOTYPE(_flockfile_debug);

WEAK_ALIAS(flockfile);
WEAK_ALIAS(ftrylockfile);
WEAK_ALIAS(funlockfile);
WEAK_ALIAS(_flockfile_debug);

void
WEAK_NAME(flockfile)(fp)
	FILE * fp;
{
}


int
WEAK_NAME(ftrylockfile)(fp)
	FILE * fp;
{

	return 0;
}

void
WEAK_NAME(funlockfile)(fp)
	FILE * fp;
{
}

void
WEAK_NAME(_flockfile_debug)(fp, fname, lineno)
	FILE * fp;
	char * fname;
	int lineno;
{
}


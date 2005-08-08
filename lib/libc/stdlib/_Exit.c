/*	$OpenBSD: src/lib/libc/stdlib/_Exit.c,v 1.2 2005/08/08 08:05:36 espie Exp $	*/

/*
 * Placed in the public domain by Todd C. Miller on January 21, 2004.
 */

#include <stdlib.h>
#include <unistd.h>

/*
 * _Exit() is the ISO/ANSI C99 equivalent of the POSIX _exit() function.
 * No atexit() handlers are called and no signal handlers are run.
 * Whether or not stdio buffers are flushed or temporary files are removed
 * is implementation-dependent.  As such it is safest to *not* flush
 * stdio buffers or remove temporary files.  This is also consistent
 * with most other implementations.
 */
void
_Exit(int status)
{
	_exit(status);
}

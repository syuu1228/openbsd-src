/*	$OpenBSD: src/usr.sbin/pkg_install/lib/Attic/pen.c,v 1.13 2003/07/04 17:31:19 avsm Exp $	*/

#ifndef lint
static const char rcsid[] = "$OpenBSD: src/usr.sbin/pkg_install/lib/Attic/pen.c,v 1.13 2003/07/04 17:31:19 avsm Exp $";
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * Routines for managing the "play pen".
 *
 */

#include <err.h>
#include "lib.h"
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <errno.h>

/* For keeping track of where we are */
static char Current[FILENAME_MAX];
static char Previous[FILENAME_MAX];

/* Backup Current and Previous into temp. strings that are later
 * restored & freed by restore_dirs
 * This is to make nested calls to makeplaypen/leave_playpen work
 */
void
save_dirs(char **c, char **p)
{
    *c=strdup(Current);
    *p=strdup(Previous);
}

/* Restore Current and Previous from temp strings that were created
 * by safe_dirs.
 * This is to make nested calls to makeplaypen/leave_playpen work
 */
void
restore_dirs(char *c, char *p)
{
    strlcpy(Current, c, sizeof(Current));  free(c);
    strlcpy(Previous, p, sizeof(Previous)); free(p);
}

char *
where_playpen(void)
{
    return Current;
}

/* Find a good place to play. */
static char *
find_play_pen(char *pen, size_t pensize, size_t sz)
{
    char *cp;
    struct stat sb;

    if (pen[0] && stat(pen, &sb) != FAIL && (min_free(pen) >= sz))
	return pen;
    else if ((cp = getenv("PKG_TMPDIR")) != NULL && stat(cp, &sb) != FAIL && (min_free(cp) >= sz))
	(void) snprintf(pen, pensize, "%s/instmp.XXXXXXXXXX", cp);
    else if ((cp = getenv("TMPDIR")) != NULL && stat(cp, &sb) != FAIL && (min_free(cp) >= sz))
	(void) snprintf(pen, pensize, "%s/instmp.XXXXXXXXXX", cp);
    else if (stat("/var/tmp", &sb) != FAIL && min_free("/var/tmp") >= sz)
	strlcpy(pen, "/var/tmp/instmp.XXXXXXXXXX", pensize);
    else if (stat("/tmp", &sb) != FAIL && min_free("/tmp") >= sz)
	strlcpy(pen, "/tmp/instmp.XXXXXXXXXX", pensize);
    else if ((stat("/usr/tmp", &sb) == SUCCESS || mkdir("/usr/tmp", 01777) == SUCCESS) && min_free("/usr/tmp") >= sz)
	strlcpy(pen, "/usr/tmp/instmp.XXXXXXXXXX", pensize);
    else {
	cleanup(0);
	errx(2,
"can't find enough temporary space to extract the files, please set your\n"
"PKG_TMPDIR environment variable to a location with at least %lu bytes\n"
"free", (u_long)sz);
	return NULL;
    }
    return pen;
}

/*
 * Make a temporary directory to play in and chdir() to it, returning
 * pathname of previous working directory.
 */
char *
make_playpen(char *pen, size_t pensize, size_t sz)
{
    if (!find_play_pen(pen, pensize, sz))
	return NULL;

    if (!mkdtemp(pen)) {
	cleanup(0);
	errx(2, "can't mkdtemp '%s'", pen);
    }
    if (Verbose) {
	if (sz)
	    fprintf(stderr, "Requested space: %lu bytes, free space: %qd bytes in %s\n", (u_long)sz, (long long)min_free(pen), pen);
    }
    if (min_free(pen) < sz) {
	rmdir(pen);
	cleanup(0);
	errx(2, "not enough free space to create '%s'.\n"
	     "Please set your PKG_TMPDIR environment variable to a location\n"
	     "with more space and\ntry the command again", pen);
    }
    if (Current[0])
	strlcpy(Previous, Current, sizeof(Previous));
    else if (!getcwd(Previous, FILENAME_MAX)) {
	cleanup(0);
	err(1, "fatal error during execution: getcwd");
    }
    if (chdir(pen) == FAIL) {
	cleanup(0);
	errx(2, "can't chdir to '%s'", pen);
    }
    strlcpy(Current, pen, sizeof(Current));
    return Previous;
}

/* Convenience routine for getting out of playpen */
void
leave_playpen(char *save)
{
    int save_errno = errno;
    void (*oldsig)(int);

    /* XXX big signal race! */
    /* Don't interrupt while we're cleaning up */
    oldsig = signal(SIGINT, SIG_IGN);
    if (Previous[0] && chdir(Previous) == FAIL) {
	cleanup(0);	/* XXX */
	warnx("can't chdir back to '%s'", Previous);
	_exit(2);
    } else if (Current[0] && strcmp(Current, Previous)) {
        if (strcmp(Current,"/")==0) {
            fprintf(stderr,"PANIC: About to rm -rf / (not doing so, aborting)\n");
            abort();	/* XXX is abort safe? */
        }
	if (vsystem("rm -rf %s", Current))
	    pwarnx("couldn't remove temporary dir '%s'", Current);
	strlcpy(Current, Previous, sizeof(Current));	/* XXX */
    }
    if (save)
	strlcpy(Previous, save, sizeof(Previous));		/* XXX */
    else
	Previous[0] = '\0';
    signal(SIGINT, oldsig);
    errno = save_errno;
}

off_t
min_free(char *tmpdir)
{
    struct statfs buf;

    if (statfs(tmpdir, &buf) != 0) {
	warn("statfs");
	return -1;
    }
    return (off_t)buf.f_bavail * (off_t)buf.f_bsize;
}

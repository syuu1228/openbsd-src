/*	$OpenBSD: src/regress/sys/fdescfs/confuse/Attic/confuse.c,v 1.1 2002/02/08 17:33:32 art Exp $	*/
/*
 *	Written by Artur Grabowski <art@openbsd.org> 2002 Public Domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <errno.h>

int
main()
{
	char fname[64];
	int fd, newfd;

	if ((fd = open("/dev/null", O_RDONLY)) < 0)
		err(1, "open(/dev/null)");

	/* Try to confuse fdescfs by making it open into itself. */
	close(fd);

	snprintf(fname, sizeof(fname), "/dev/fd/%d", fd);

	if ((newfd = open(fname, O_RDONLY)) == fd)
		errx(1, "open of %s to %d succeeded, beware.", fname, fd);

	if (newfd >= 0)
		errx(1, "open(%s) gave us the unexpected %d", fname, fd);

	if (errno == ENOENT)
		err(1, "open(%s)", fname);

	if (errno == ENXIO)
		errx(1, "no support for fdesc in kernel"); 

	warn("errno was (%d)", errno);

	return 0;
}

/*	$OpenBSD: src/sys/arch/sun3/stand/libsa/Attic/devopen.c,v 1.4 2002/03/14 03:16:01 millert Exp $	*/


#include <sys/param.h>
#include <machine/mon.h>
#include <stand.h>
#include "promboot.h"

/*
 * Open the device named by the combined device/file name
 * given as the "fname" arg, something like: "sd()bsd"
 *
 * However, Sun PROMs don't really let you choose which
 * device you will talk to.  You can only open the device
 * that was used to load the boot program.  Therefore, we
 * do not accept a "device" part in the "fname" string.
 * Pass the PROM device name to open in case it needs it.
 */
int
devopen(f, fname, file)
	struct open_file *f;
	const char *fname;
	char **file;
{
	struct devsw *dp;
	int error;

	*file = (char *)fname;
	dp = &devsw[0];
	f->f_dev = dp;
	error = (*dp->dv_open)(f, prom_bootdev);

	return (error);
}

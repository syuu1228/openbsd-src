/*	$OpenBSD: src/usr.bin/top/Attic/version.c,v 1.1 1997/08/14 14:00:28 downsj Exp $	*/

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

#include "top.h"
#include "patchlevel.h"

static char version[16];

char *version_string()

{
    snprintf(version, sizeof(version), "%d.%d", VERSION, PATCHLEVEL);
#ifdef BETA
    strcat(version, BETA);
#endif
    return(version);
}

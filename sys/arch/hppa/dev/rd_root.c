/*	$OpenBSD: src/sys/arch/hppa/dev/Attic/rd_root.c,v 1.2 1999/04/20 20:23:24 mickey Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1995 Gordon W. Ross
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <dev/ramdisk.h>

extern int boothowto;
extern u_int32_t rd_root_size;
extern char rd_root_image[];

#ifndef MINIROOTSIZE
#define MINIROOTSIZE 512
#endif

#define ROOTBYTES (MINIROOTSIZE << DEV_BSHIFT)

/*
 * This is called during autoconfig.
 */
void
rd_attach_hook(unit, rd)
	int unit;
	struct rd_conf *rd;
{
	if (unit == 0) {
		/* Setup root ramdisk */
		rd->rd_addr = rd_root_image;
		rd->rd_size = rd_root_size;
		rd->rd_type = RD_KMEM_FIXED;
		printf("rd%d: internal %dK image area\n", unit,
		       rd_root_size / 1024);
	}
}

/*
 * This is called during open (i.e. mountroot)
 */
void
rd_open_hook(unit, rd)
	int unit;
	struct rd_conf *rd;
{

	if (unit == 0) {
		/* The root ramdisk only works single-user. */
		boothowto |= RB_SINGLE;
	}
}

/*
 * This array will be patched to contain a file-system image.
 * XXX there is an overflow bug in gnu ld-hppa, so keep this down
 * the of .data segment (see ld.script) to avoid. will fix this later
 */
u_int32_t rd_root_size = ROOTBYTES;
char rd_root_image[ROOTBYTES] = "|This is the root ramdisk!\n";

/*	$NetBSD: rootfil.c,v 1.11 1996/04/08 18:32:54 ragge Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */
 /* All bugs are subject to removal without further notice */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/macros.h>
#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/pte.h>
#include <machine/cpu.h>

#include <vax/uba/ubavar.h>

#include "uda.h"
#include "hp.h"

#define DOSWAP                  /* Change swdevt, argdev, and dumpdev too */
u_long  bootdev;                /* should be dev_t, but not until 32 bits */
extern dev_t rootdev, dumpdev;

static  char devname[][2] = {
        {'h','p'},        /* 0 = hp */
        {0,0},            /* 1 = ht */
        {'u','p'},        /* 2 = up */
        {'r','k'},        /* 3 = hk */
        {0,0},            /* 4 = sw */
        {0,0},            /* 5 = tm */
        {0,0},            /* 6 = ts */
        {0,0},            /* 7 = mt */
        {0,0},            /* 8 = tu */
        {'r','a'},        /* 9 = ra */
        {0,0},            /* 10 = ut */
        {'r','b'},        /* 11 = rb */
        {0,0},            /* 12 = uu */
        {0,0},            /* 13 = rx */
        {'r','l'},        /* 14 = rl */
        {0,0},            /* 15 = tmscp */
        {'k','r'},        /* 16 = ra on kdb50 */
};

#define PARTITIONMASK   0x7
#define PARTITIONSHIFT  3

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
setroot()
{
        int  majdev, mindev, unit, part, controller, adaptor;
        dev_t temp = 0, orootdev;
#if NUDA > 0
	extern struct uba_device ubdinit[];
#endif
        struct swdevt *swp;
	extern int boothowto;

        if (boothowto & RB_DFLTROOT ||
            (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
                return;
        majdev = B_TYPE(bootdev);
        if (majdev >= sizeof(devname) / sizeof(devname[0]))
                return;
        adaptor = B_ADAPTOR(bootdev);
        controller = B_CONTROLLER(bootdev);
        part = B_PARTITION(bootdev);
        unit = B_UNIT(bootdev);
        if (majdev == 0) {      /* MBA device */
#if NHP > 0
		mindev = hp_getdev(adaptor, unit);
		if (mindev < 0)
			return;
#else
                return;
#endif
        } else {
                register struct uba_device *ubap;

                for (ubap = ubdinit; ubap->ui_driver; ubap++){
                        if (ubap->ui_alive && ubap->ui_slave == unit &&
                           ubap->ui_ctlr == controller &&
                           ubap->ui_ubanum == adaptor &&
                           ubap->ui_driver->ud_dname[0] == devname[majdev][0] &&
                           ubap->ui_driver->ud_dname[1] == devname[majdev][1])
                                break;
		}
                if (ubap->ui_driver == 0)
                        return;
                mindev = ubap->ui_unit;
        }
        mindev = (mindev << PARTITIONSHIFT) + part;
        orootdev = rootdev;
        rootdev = makedev(majdev, mindev);
        /*
         * If the original rootdev is the same as the one
         * just calculated, don't need to adjust the swap configuration.
         */
        if (rootdev == orootdev)
                return;

        printf("Changing root device to %c%c%d%c\n",
                devname[majdev][0], devname[majdev][1],
                mindev >> PARTITIONSHIFT, part + 'a');

#ifdef DOSWAP
        mindev &= ~PARTITIONMASK;
        for (swp = swdevt; swp->sw_dev; swp++) {
                if (majdev == major(swp->sw_dev) &&
                    mindev == (minor(swp->sw_dev) & ~PARTITIONMASK)) {
                        temp = swdevt[0].sw_dev;
                        swdevt[0].sw_dev = swp->sw_dev;
                        swp->sw_dev = temp;
                        break;
                }
        }
        if (swp->sw_dev == 0)
                return;

        /*
         * If argdev and dumpdev were the same as the old primary swap
         * device, move them to the new primary swap device.
         */
        if (temp == dumpdev)
                dumpdev = swdevt[0].sw_dev;
        panic("autoconf.c: argdev\n");
/*      if (temp == argdev)
                argdev = swdevt[0].sw_dev; */
#endif
}

/*
 * Configure swap space and related parameters.
 */
void
swapconf()
{
        register struct swdevt *swp;
        register int nblks;

        for (swp = swdevt; swp->sw_dev; swp++)
		if (swp->sw_dev != NODEV &&bdevsw[major(swp->sw_dev)].d_psize){
                        nblks =
                          (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
                        if (nblks != -1 &&
                            (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
                                swp->sw_nblks = nblks;
                }
        dumpconf();
}

/*	$OpenBSD: src/sys/arch/sparc/include/conf.h,v 1.22 2010/07/21 15:40:04 deraadt Exp $	*/
/*	$NetBSD: conf.h,v 1.8 1996/12/31 07:12:43 mrg Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
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

#define mmread mmrw
#define mmwrite mmrw
cdev_decl(mm);

/* open, close, ioctl */
#define	cdev_openprom_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, 0, selfalse, \
	(dev_type_mmap((*))) enodev }

cdev_decl(openprom);

cdev_decl(zs);

bdev_decl(fd);
cdev_decl(fd);

/* open, close, read, write, ioctl, poll */
#define	cdev_gen_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	0, dev_init(c,n,poll), (dev_type_mmap((*))) enodev }

bdev_decl(xd);
cdev_decl(xd);

bdev_decl(xy);
cdev_decl(xy);

bdev_decl(presto);
cdev_decl(presto);

/* open, close, write, ioctl */
#define	cdev_bpp_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	0, seltrue, (dev_type_mmap((*))) enodev }

cdev_decl(bpp);

cdev_decl(mtty);
cdev_decl(mbpp);

cdev_decl(stty);
cdev_decl(sbpp);

/* open, close, ioctl */
#define	cdev_scf_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, 0, seltrue, \
	(dev_type_mmap((*))) enodev }
cdev_decl(scf);

cdev_decl(com);

/* open, close, ioctl, read, write */
#define	cdev_flash_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read),	\
	dev_init(c,n,write), dev_init(c,n,ioctl),			\
	(dev_type_stop((*))) nullop, 0, seltrue, (dev_type_mmap((*))) enodev }
cdev_decl(flash);

#define	cdev_fga_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, 0, seltrue, \
	(dev_type_mmap((*))) enodev }
cdev_decl(fga);

#define	cdev_daadio_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) nullop, 0, seltrue, \
	(dev_type_mmap((*))) enodev }
cdev_decl(daadio);

/* open, close, write, ioctl, kqueue */
#define cdev_apm_init(c,n) { \
        dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
        (dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, selfalse, \
	(dev_type_mmap((*))) enodev, 0, D_KQFILTER, dev_init(c,n,kqfilter) }
cdev_decl(apm);

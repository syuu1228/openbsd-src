/*	$OpenBSD: src/sys/arch/i386/stand/libsa/libsa.h,v 1.30 2000/03/05 18:40:59 niklas Exp $	*/

/*
 * Copyright (c) 1996-1999 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <machine/biosvar.h>

#define	EXEC_AOUT

#define	DEFAULT_KERNEL_ADDRESS	0x100000

void gateA20 __P((int));

void smpprobe __P((void));
void memprobe __P((void));
void diskprobe __P((void));
void apmprobe __P((void));
void apmcheck __P((void));
void pciprobe __P((void));
void dump_biosmem __P((bios_memmap_t *));
int mem_add __P((long, long));
int mem_delete __P((long, long));
void mem_pass __P((void));

void devboot __P((dev_t, char *));
void machdep __P((void));

extern const char bdevs[][4];
extern const int nbdevs;
extern u_int cnvmem, extmem; /* XXX global pass memprobe()->machdep_start() */

/* diskprobe.c */
extern bios_diskinfo_t bios_diskinfo[];
extern u_int32_t bios_cksumlen;

#define MACHINE_CMD	cmd_machine /* we have i386 specific sommands */

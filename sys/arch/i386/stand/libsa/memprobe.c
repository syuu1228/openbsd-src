/*	$OpenBSD: src/sys/arch/i386/stand/libsa/memprobe.c,v 1.11 1997/09/02 17:00:42 mickey Exp $	*/

/*
 * Copyright (c) 1997 Tobias Weingartner
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
 *	This product includes software developed by Tobias Weingartner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <machine/biosvar.h>
#include "libsa.h"

static int addrprobe __P((int));

void
memprobe()
{
	int ram;

	__asm __volatile(DOINT(0x12) : "=a" (BIOS_vars.bios_cnvmem)
			 :: "%ecx", "%edx", "cc");
	BIOS_vars.bios_cnvmem &= 0xffff;

	printf("%d Kb conventional memory.\n", BIOS_vars.bios_cnvmem);

	/* probe extended memory
	 *
	 * There is no need to do this in assembly language.  This is
	 * much easier to debug in C anyways.
	 */
	for(ram = 1024; ram < 512*1024; ram += 4){

		if(!(ram % 64))
			printf("Probing memory: %d KB\r", ram-1024);
		if(addrprobe(ram))
			break;
	}

	printf("%d Kb extended memory.\n", ram-1024);
	BIOS_vars.bios_extmem = ram - 1024;
}

/* addrprobe(kloc): Probe memory at address kloc * 1024.
 *
 * This is a hack, but it seems to work ok.  Maybe this is
 * the *real* way that you are supposed to do probing???
 *
 * Modify the original a bit.  We write everything first, and
 * then test for the values.  This should croak on machines that
 * return values just written on non-existent memory...
 *
 * BTW: These machines are pretty broken IMHO.
 */
static int addrprobe(int kloc){
	volatile int *loc, i;
	static const int pat[] = {
		0x00000000, 0xFFFFFFFF,
		0x01010101, 0x10101010,
		0x55555555, 0xCCCCCCCC
	};

	/* Get location */
	loc = (int *)(kloc * 1024);

	/* Probe address */
	for(i = 0; i < NENTS(pat); i++){
		*loc = pat[i];
		if(*loc != pat[i])
			return (1);
	}

	/* Write address */
	for(i = 0; i < NENTS(pat); i++)
		loc[i] = pat[i];

	/* Read address */
	for(i = 0; i < NENTS(pat); i++)
		if(loc[i] != pat[i])
			return (1);

	return (0);
}



/*	$OpenBSD: src/sys/arch/i386/i386/apic.c,v 1.2 2004/06/13 21:49:15 niklas Exp $	*/
/* $NetBSD: apic.c,v 1.1.2.2 2000/02/21 18:51:00 sommerfeld Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/apicvar.h>


const char redirlofmt[] = "\177\20"
	"f\0\10vector\0"
	"f\10\3delmode\0"
	"b\13logical\0"
	"b\14pending\0"
	"b\15actlo\0"
	"b\16irrpending\0"
	"b\17level\0"
	"b\20masked\0"
	"f\22\1dest\0" "=\1self" "=\2all" "=\3all-others";

const char redirhifmt[] = "\177\20"
	"f\30\10target\0";

void
apic_format_redir(where1, where2, idx, redirhi, redirlo)
	char *where1;
	char *where2;
	int idx;
	u_int32_t redirhi;
	u_int32_t redirlo;
{
	printf("%s: %s%d 0x%x", where1, where2, idx, redirlo);

	if ((redirlo & LAPIC_DEST_MASK) == 0)
		printf(" 0x%x", redirhi);

	printf("\n");
}


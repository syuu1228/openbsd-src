/*	$OpenBSD: src/sys/dev/pci/pciide_cy693_reg.h,v 1.1 1999/07/18 21:25:20 csapuntz Exp $	*/
/*	$NetBSD: pciide_cy693_reg.h,v 1.2 1998/12/03 14:06:16 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Manuel Bouyer.
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
 */

/*
 * Registers definitions for Contaq/Cypress's CY82693U PCI IDE controller.
 * Available from http://www.cypress.com/japan/prodgate/chip/cy82c693.html
 * This chip has 2 PCI IDE functions, each of them has only one channel
 * So there's no primary/secodary distinction in the registers defs.
 */

/* IDE control register */
#define CY_CTRL 0x40
#define CY_CTRL_RETRY			0x00002000
#define CY_CTRL_SLAVE_PREFETCH		0x00000400
#define CY_CTRL_POSTWRITE		0x00000200
#define	CY_CTRL_PREFETCH(drive)		(0x00000100 << (2 * (drive)))
#define CY_CTRL_POSTWRITE_LENGTH_MASK	0x00000030
#define CY_CTRL_POSTWRITE_LENGTH_OFF    4
#define CY_CTRL_PREFETCH_LENGTH_MASK	0x00000003
#define CY_CTRL_PREFETCH_LENGTH_OFF	0

/* IDE addr setup control register */
#define CY_ADDR_CTRL 0x48
#define CY_ADDR_CTRL_SETUP_OFF(drive)  (4 * (drive))
#define CY_ADDR_CTRL_SETUP_MASK(drive) \
	(0x00000007 << CY_ADDR_CTRL_SETUP_OFF(drive))

/* command control register */
#define CY_CMD_CTRL 0x4c
#define CY_CMD_CTRL_IOW_PULSE_OFF(drive)	(12 + 16 * (drive))
#define CY_CMD_CTRL_IOW_REC_OFF(drive)		(8 + 16 * (drive))
#define CY_CMD_CTRL_IOR_PULSE_OFF(drive)	(4 + 16 * (drive))
#define CY_CMD_CTRL_IOR_REC_OFF(drive)		(0 + 16 * (drive))

static int8_t cy_pio_pulse[] = {9, 4, 3, 2, 2};
static int8_t cy_pio_rec[] =   {9, 7, 4, 2, 0};
#ifdef unused
static int8_t cy_dma_pulse[] = {7, 2, 2};
static int8_t cy_dma_rec[] =   {7, 1, 0};
#endif

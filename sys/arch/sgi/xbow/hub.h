/*	$OpenBSD: src/sys/arch/sgi/xbow/hub.h,v 1.2 2009/05/28 18:02:43 miod Exp $	*/

/*
 * Copyright (c) 2009 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * IP27 specific registers
 */

/*
 * HUB space (very incomplete)
 */

/*
 * HUB PI
 */

#define	HUB_CPU_NUMBER			0x00000020

#define	HUB_CPU0_PRESENT		0x00000040
#define	HUB_CPU1_PRESENT		0x00000048
#define	HUB_CPU0_ENABLED		0x00000050
#define	HUB_CPU1_ENABLED		0x00000058

#define	HUB_IR_CHANGE			0x00000090
#define	HUB_IR_SET				0x100
#define	HUB_IR_CLR				0x000
#define	HUB_IR0				0x00000098
#define	HUB_IR1				0x000000a0
#define	HUB_CPU0_IMR0			0x000000a8
#define	HUB_CPU0_IMR1			0x000000b0
#define	HUB_CPU1_IMR0			0x000000b8
#define	HUB_CPU1_IMR1			0x000000c0


/*
 * HUB NI
 */

#define	HUB_NI_IP27			0x00600000
#define	HUB_NI_IP35			0x00680000

#define	HUB_NI_STATUS			0x00000000
#define	HUB_NI_RESET			0x00000008
#define	RESET_ACTION				0x01
#define	RESET_PORT				0x02
#define	RESET_LOCAL				0x04
#define	HUB_NI_RESET_ENABLE		0x00000010
#define	RESET_ENABLE				0x01

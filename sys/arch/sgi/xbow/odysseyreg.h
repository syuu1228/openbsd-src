/*	$OpenBSD: src/sys/arch/sgi/xbow/odysseyreg.h,v 1.2 2012/04/16 22:17:16 miod Exp $ */
/*
 * Copyright (c) 2009, 2010 Joel Sing <jsing@openbsd.org>
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
 * Register details for the SGI VPro (aka Odyssey) Graphics Card.
 */

#define	ODYSSEY_REG_OFFSET		0x00000000
#define	ODYSSEY_REG_SIZE		0x00410000

#define ODYSSEY_STATUS			0x00001064
#define  ODYSSEY_STATUS_CMD_FIFO_HIGH	0x00008000
#define  ODYSSEY_STATUS_CMD_FIFO_LOW	0x00020000
#define ODYSSEY_DBE_STATUS		0x0000106c

#define ODYSSEY_CMD_FIFO		0x00110000
#define ODYSSEY_DATA_FIFO		0x00400000

/*
 * OpenGL Commands.
 */

#define OPENGL_BEGIN			0x00014400
#define OPENGL_END			0x00014001
#define OPENGL_LOGIC_OP			0x00010422
#define OPENGL_VERTEX_2I		0x8080c800
#define OPENGL_COLOR_3UB		0xc580cc08

#define OPENGL_QUADS			0x00000007

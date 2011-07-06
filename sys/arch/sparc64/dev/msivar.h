/*	$OpenBSD: src/sys/arch/sparc64/dev/msivar.h,v 1.1 2011/07/06 05:35:53 kettenis Exp $	*/
/*
 * Copyright (c) 2011 Mark Kettenis <kettenis@openbsd.org>
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

struct msi_eq *msi_eq_alloc(bus_dma_tag_t, int);
void msi_eq_free(bus_dma_tag_t t, struct msi_eq *);

struct msi_eq {
	bus_dmamap_t	meq_map;
	bus_dma_segment_t meq_seg;
	caddr_t		meq_va;
	int		meq_nentries;
};

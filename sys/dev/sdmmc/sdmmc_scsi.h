/*	$OpenBSD: src/sys/dev/sdmmc/sdmmc_scsi.h,v 1.2 2006/06/01 21:53:41 uwe Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
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

#ifndef _SDMMC_SCSI_H_
#define _SDMMC_SCSI_H_

struct sdmmc_softc;

struct sdmmc_scsi_target {
	struct sdmmc_function *card;
};

struct sdmmc_scsi_softc {
	struct scsi_adapter sc_adapter;
	struct scsi_link sc_link;
	struct device *sc_child;
	struct sdmmc_scsi_target *sc_tgt;
	int sc_ntargets;
};

void	sdmmc_scsi_attach(struct sdmmc_softc *);
void	sdmmc_scsi_detach(struct sdmmc_softc *);

#endif

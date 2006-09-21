/*	$OpenBSD: src/sys/dev/ic/mpi.c,v 1.70 2006/09/21 09:42:27 dlg Exp $ */

/*
 * Copyright (c) 2005, 2006 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2005 Marco Peereboom <marco@openbsd.org>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/ic/mpireg.h>
#include <dev/ic/mpivar.h>

#ifdef MPI_DEBUG
uint32_t	mpi_debug = 0
/*		    | MPI_D_CMD */
/*		    | MPI_D_INTR */
/*		    | MPI_D_MISC */
/*		    | MPI_D_DMA */
/*		    | MPI_D_IOCTL */
/*		    | MPI_D_RW */
/*		    | MPI_D_MEM */
/*		    | MPI_D_CCB */
/*		    | MPI_D_PPR */
/*		    | MPI_D_RAID */
		;
#endif

struct cfdriver mpi_cd = {
	NULL, "mpi", DV_DULL
};

int			mpi_scsi_cmd(struct scsi_xfer *);
void			mpi_scsi_cmd_done(struct mpi_ccb *);
void			mpi_minphys(struct buf *bp);
int			mpi_scsi_ioctl(struct scsi_link *, u_long, caddr_t,
			    int, struct proc *);

struct scsi_adapter mpi_switch = {
	mpi_scsi_cmd, mpi_minphys, NULL, NULL, mpi_scsi_ioctl
};

struct scsi_device mpi_dev = {
	NULL, NULL, NULL, NULL
};

struct mpi_dmamem	*mpi_dmamem_alloc(struct mpi_softc *, size_t);
void			mpi_dmamem_free(struct mpi_softc *,
			    struct mpi_dmamem *);
int			mpi_alloc_ccbs(struct mpi_softc *);
struct mpi_ccb		*mpi_get_ccb(struct mpi_softc *);
void			mpi_put_ccb(struct mpi_softc *, struct mpi_ccb *);
int			mpi_alloc_replies(struct mpi_softc *);
void			mpi_push_replies(struct mpi_softc *);

void			mpi_start(struct mpi_softc *, struct mpi_ccb *);
int			mpi_complete(struct mpi_softc *, struct mpi_ccb *, int);
int			mpi_poll(struct mpi_softc *, struct mpi_ccb *, int);
int			mpi_reply(struct mpi_softc *, u_int32_t);

void			mpi_fc_print(struct mpi_softc *);
void			mpi_squash_ppr(struct mpi_softc *);
void			mpi_run_ppr(struct mpi_softc *);
int			mpi_ppr(struct mpi_softc *, struct scsi_link *,
			    struct mpi_cfg_raid_physdisk *, int, int, int);
int			mpi_inq(struct mpi_softc *, u_int16_t, int);

void			mpi_timeout_xs(void *);
int			mpi_load_xs(struct mpi_ccb *);

u_int32_t		mpi_read(struct mpi_softc *, bus_size_t);
void			mpi_write(struct mpi_softc *, bus_size_t, u_int32_t);
int			mpi_wait_eq(struct mpi_softc *, bus_size_t, u_int32_t,
			    u_int32_t);
int			mpi_wait_ne(struct mpi_softc *, bus_size_t, u_int32_t,
			    u_int32_t);

int			mpi_init(struct mpi_softc *);
int			mpi_reset_soft(struct mpi_softc *);
int			mpi_reset_hard(struct mpi_softc *);

int			mpi_handshake_send(struct mpi_softc *, void *, size_t);
int			mpi_handshake_recv_dword(struct mpi_softc *,
			    u_int32_t *);
int			mpi_handshake_recv(struct mpi_softc *, void *, size_t);

void			mpi_empty_done(struct mpi_ccb *);

int			mpi_iocinit(struct mpi_softc *);
int			mpi_iocfacts(struct mpi_softc *);
int			mpi_portfacts(struct mpi_softc *);
int			mpi_eventnotify(struct mpi_softc *);
void			mpi_eventnotify_done(struct mpi_ccb *);
int			mpi_portenable(struct mpi_softc *);
void			mpi_get_raid(struct mpi_softc *);
int			mpi_fwupload(struct mpi_softc *);

int			mpi_cfg_header(struct mpi_softc *, u_int8_t, u_int8_t,
			    u_int32_t, struct mpi_cfg_hdr *);
int			mpi_cfg_page(struct mpi_softc *, u_int32_t,
			    struct mpi_cfg_hdr *, int, void *, size_t);

#define DEVNAME(s)		((s)->sc_dev.dv_xname)

#define	dwordsof(s)		(sizeof(s) / sizeof(u_int32_t))
#define	sizeofa(s)		(sizeof(s) / sizeof((s)[0]))

#define mpi_read_db(s)		mpi_read((s), MPI_DOORBELL)
#define mpi_write_db(s, v)	mpi_write((s), MPI_DOORBELL, (v))
#define mpi_read_intr(s)	mpi_read((s), MPI_INTR_STATUS)
#define mpi_write_intr(s, v)	mpi_write((s), MPI_INTR_STATUS, (v))
#define mpi_pop_reply(s)	mpi_read((s), MPI_REPLY_QUEUE)
#define mpi_push_reply(s, v)	mpi_write((s), MPI_REPLY_QUEUE, (v))

#define mpi_wait_db_int(s)	mpi_wait_ne((s), MPI_INTR_STATUS, \
				    MPI_INTR_STATUS_DOORBELL, 0)
#define mpi_wait_db_ack(s)	mpi_wait_eq((s), MPI_INTR_STATUS, \
				    MPI_INTR_STATUS_IOCDOORBELL, 0)

int
mpi_attach(struct mpi_softc *sc)
{
	struct mpi_ccb			*ccb;

	printf("\n");

	/* disable interrupts */
	mpi_write(sc, MPI_INTR_MASK,
	    MPI_INTR_MASK_REPLY | MPI_INTR_MASK_DOORBELL);

	if (mpi_init(sc) != 0) {
		printf("%s: unable to initialise\n", DEVNAME(sc));
		return (1);
	}

	if (mpi_iocfacts(sc) != 0) {
		printf("%s: unable to get iocfacts\n", DEVNAME(sc));
		return (1);
	}

	if (mpi_alloc_ccbs(sc) != 0) {
		/* error already printed */
		return (1);
	}

	if (mpi_alloc_replies(sc) != 0) {
		printf("%s: unable to allocate reply space\n", DEVNAME(sc));
		goto free_ccbs;
	}

	if (mpi_iocinit(sc) != 0) {
		printf("%s: unable to send iocinit\n", DEVNAME(sc));
		goto free_ccbs;
	}

	/* spin until we're operational */
	if (mpi_wait_eq(sc, MPI_DOORBELL, MPI_DOORBELL_STATE,
	    MPI_DOORBELL_STATE_OPER) != 0) {
		printf("%s: state: 0x%08x\n", DEVNAME(sc),
		    mpi_read_db(sc) & MPI_DOORBELL_STATE);
		printf("%s: operational state timeout\n", DEVNAME(sc));
		goto free_ccbs;
	}

	mpi_push_replies(sc);

	if (mpi_portfacts(sc) != 0) {
		printf("%s: unable to get portfacts\n", DEVNAME(sc));
		goto free_replies;
	}

#if notyet
	if (mpi_eventnotify(sc) != 0) {
		printf("%s: unable to get portfacts\n", DEVNAME(sc));
		goto free_replies;
	}
#endif

	if (mpi_portenable(sc) != 0) {
		printf("%s: unable to enable port\n", DEVNAME(sc));
		goto free_replies;
	}

	if (mpi_fwupload(sc) != 0) {
		printf("%s: unable to upload firmware\n", DEVNAME(sc));
		goto free_replies;
	}

	if (sc->sc_porttype == MPI_PORTFACTS_PORTTYPE_SCSI)
		mpi_squash_ppr(sc);

	/* we should be good to go now, attach scsibus */
	sc->sc_link.device = &mpi_dev;
	sc->sc_link.adapter = &mpi_switch;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_target = sc->sc_target;
	sc->sc_link.adapter_buswidth = sc->sc_buswidth;
	sc->sc_link.openings = sc->sc_maxcmds / sc->sc_buswidth;

	/* config_found() returns the scsibus we should attach to */
	sc->sc_scsibus = (struct scsibus_softc *) config_found(&sc->sc_dev,
	    &sc->sc_link, scsiprint);

	/* get raid pages */
	mpi_get_raid(sc);

	/* do domain validation */
	if (sc->sc_porttype == MPI_PORTFACTS_PORTTYPE_SCSI)
		mpi_run_ppr(sc);
	if (sc->sc_porttype == MPI_PORTFACTS_PORTTYPE_FC)
		mpi_fc_print(sc);

	/* enable interrupts */
	mpi_write(sc, MPI_INTR_MASK, MPI_INTR_MASK_DOORBELL);

	return (0);

free_replies:
	bus_dmamap_sync(sc->sc_dmat, MPI_DMA_MAP(sc->sc_replies),
	    0, PAGE_SIZE, BUS_DMASYNC_POSTREAD);
	mpi_dmamem_free(sc, sc->sc_replies);
free_ccbs:
	while ((ccb = mpi_get_ccb(sc)) != NULL)
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);
	mpi_dmamem_free(sc, sc->sc_requests);
	free(sc->sc_ccbs, M_DEVBUF);

	return(1);
}

void
mpi_fc_print(struct mpi_softc *sc)
{
	struct mpi_cfg_hdr		hdr;
	struct mpi_cfg_fc_port_pg0	pg;
	struct mpi_cfg_fc_device_pg0	dpg;
	struct device			*dev;
	struct scsibus_softc		*ssc;
	struct scsi_link		*link;
	int				i;
	u_int32_t			btid;

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_FC_PORT, 0, 0,
	    &hdr) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_fc_print unable to fetch "
		    "FC port header 0\n", DEVNAME(sc));
		return;
	}

	if (mpi_cfg_page(sc, 0, &hdr, 1, &pg, sizeof(pg)) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_fc_print unable to fetch "
		    "FC port page 0\n",
		    DEVNAME(sc));
		return;
	}

	DNPRINTF(MPI_D_MISC, "%s: at: %dGHz WWNN: %016llx WWPN: %016llx\n",
	    DEVNAME(sc), letoh32(pg.current_speed), letoh64(pg.wwnn),
	    letoh64(pg.wwpn));

	TAILQ_FOREACH(dev, &alldevs, dv_list) {
		if (dev->dv_parent == &sc->sc_dev)
			break;
	}

	/* im too nice to punish idiots who don't configure scsibus */
	if (dev == NULL)
		return;

	ssc = (struct scsibus_softc *)dev;
	for (i = 0; i < sc->sc_link.adapter_buswidth; i++) {

		link = ssc->sc_link[i][0];

		if (link == NULL)
			continue;

		btid = i | MPI_PAGE_ADDRESS_FC_BTID;
		if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_FC_DEV, 0,
		    btid, &hdr) != 0) {
			DNPRINTF(MPI_D_MISC, "%s: mpi_fc_print unable to fetch "
			    "device header 0\n", DEVNAME(sc));
			return;
		}

		bzero(&dpg, sizeof(dpg));
		if (mpi_cfg_page(sc, btid, &hdr, 1, &dpg, sizeof(dpg)) != 0) {
			DNPRINTF(MPI_D_MISC, "%s: mpi_fc_print unable to fetch "
			    "device page 0\n", DEVNAME(sc));
			continue;
		}

		link->port_wwn = letoh64(dpg.wwpn);
		link->node_wwn = letoh64(dpg.wwnn);

		DNPRINTF(MPI_D_MISC, "%s: target %d WWNN: %016llx "
		    "WWPN: %016llx\n", DEVNAME(sc), i,
		    letoh64(dpg.wwnn), letoh64(dpg.wwpn));
	}
}

void
mpi_squash_ppr(struct mpi_softc *sc)
{
	struct mpi_cfg_hdr		hdr;
	struct mpi_cfg_spi_dev_pg1	page;
	int				i;

	DNPRINTF(MPI_D_PPR, "%s: mpi_squash_ppr\n", DEVNAME(sc));

	for (i = 0; i < sc->sc_buswidth; i++) {
		if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_SCSI_SPI_DEV,
		    1, i, &hdr) != 0)
			return;

		if (mpi_cfg_page(sc, i, &hdr, 1, &page, sizeof(page)) != 0)
			return;

		DNPRINTF(MPI_D_PPR, "%s:  target: %d req_params1: 0x%02x "
		    "req_offset: 0x%02x req_period: 0x%02x "
		    "req_params2: 0x%02x conf: 0x%08x\n", DEVNAME(sc), i,
		    page.req_params1, page.req_offset, page.req_period,
		    page.req_params2, letoh32(page.configuration));

		page.req_params1 = 0x0;
		page.req_offset = 0x0;
		page.req_period = 0x0;
		page.req_params2 = 0x0;
		page.configuration = htole32(0x0);

		if (mpi_cfg_page(sc, i, &hdr, 0, &page, sizeof(page)) != 0)
			return;
	}
}

void
mpi_run_ppr(struct mpi_softc *sc)
{
	struct mpi_cfg_hdr		hdr;
	struct mpi_cfg_spi_port_pg0	port_pg;
	struct mpi_cfg_ioc_pg3		*physdisk_pg;
	struct mpi_cfg_raid_physdisk	*physdisk_list, *physdisk;
	size_t				pagelen;
	struct scsi_link		*link;
	int				i, tries;

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_SCSI_SPI_PORT, 0, 0x0,
	    &hdr) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_run_ppr unable to fetch header\n",
		    DEVNAME(sc));
		return;
	}

	if (mpi_cfg_page(sc, 0x0, &hdr, 1, &port_pg, sizeof(port_pg)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_run_ppr unable to fetch page\n",
		    DEVNAME(sc));
		return;
	}

	for (i = 0; i < sc->sc_buswidth; i++) {
		link = sc->sc_scsibus->sc_link[i][0];
		if (link == NULL)
			continue;

		/* do not ppr volumes */
		if (link->flags & SDEV_VIRTUAL)
			continue;

		tries = 0;
		while (mpi_ppr(sc, link, NULL, port_pg.min_period,
		    port_pg.max_offset, tries) == EAGAIN)
			tries++;
	}

	if ((sc->sc_flags & MPI_F_RAID) == 0)
		return;

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_IOC, 3, 0x0,
	    &hdr) != 0) {
		DNPRINTF(MPI_D_RAID|MPI_D_PPR, "%s: mpi_run_ppr unable to "
		    "fetch ioc pg 3 header\n", DEVNAME(sc));
		return;
	}

	pagelen = hdr.page_length * 4; /* dwords to bytes */
	physdisk_pg = malloc(pagelen, M_TEMP, M_WAITOK);
	if (physdisk_pg == NULL) {
		DNPRINTF(MPI_D_RAID|MPI_D_PPR, "%s: mpi_run_ppr unable to "
		    "allocate ioc pg 3\n", DEVNAME(sc));
		return;
	}
	physdisk_list = (struct mpi_cfg_raid_physdisk *)(physdisk_pg + 1);

	if (mpi_cfg_page(sc, 0, &hdr, 1, physdisk_pg, pagelen) != 0) {
		DNPRINTF(MPI_D_PPR|MPI_D_PPR, "%s: mpi_run_ppr unable to "
		    "fetch ioc page 3\n", DEVNAME(sc));
		goto out;
	}

	DNPRINTF(MPI_D_PPR|MPI_D_PPR, "%s:  no_phys_disks: %d\n", DEVNAME(sc),
	    physdisk_pg->no_phys_disks);

	for (i = 0; i < physdisk_pg->no_phys_disks; i++) {
		physdisk = &physdisk_list[i];

		DNPRINTF(MPI_D_PPR|MPI_D_PPR, "%s:  id: %d bus: %d ioc: %d "
		    "num: %d\n", DEVNAME(sc), physdisk->phys_disk_id,
		    physdisk->phys_disk_bus, physdisk->phys_disk_ioc,
		    physdisk->phys_disk_num);

		if (physdisk->phys_disk_ioc != sc->sc_ioc_number)
			continue;

		tries = 0;
		while (mpi_ppr(sc, NULL, physdisk, port_pg.min_period,
		    port_pg.max_offset, tries) == EAGAIN)
			tries++;
	}

out:
	free(physdisk_pg, M_TEMP);
}

int
mpi_ppr(struct mpi_softc *sc, struct scsi_link *link,
    struct mpi_cfg_raid_physdisk *physdisk, int period, int offset, int try)
{
	struct mpi_cfg_hdr		hdr0, hdr1;
	struct mpi_cfg_spi_dev_pg0	pg0;
	struct mpi_cfg_spi_dev_pg1	pg1;
	u_int32_t			address;
	int				id;
	int				raid = 0;

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr period: %d offset: %d try: %d "
	    "link quirks: 0x%x\n", DEVNAME(sc), period, offset, try,
	    link->quirks);

	if (try >= 3)
		return (EIO);

	if (physdisk == NULL) {
		if ((link->inqdata.device & SID_TYPE) == T_PROCESSOR)
			return (EIO);

		address = link->target;
		id = link->target;
	} else {
		raid = 1;
		address = (physdisk->phys_disk_bus << 8) |
		    (physdisk->phys_disk_id);
		id = physdisk->phys_disk_num;
	}

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_SCSI_SPI_DEV, 0,
	    address, &hdr0) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to fetch header 0\n",
		    DEVNAME(sc));
		return (EIO);
	}

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_SCSI_SPI_DEV, 1,
	    address, &hdr1) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to fetch header 1\n",
		    DEVNAME(sc));
		return (EIO);
	}

#ifdef MPI_DEBUG
	if (mpi_cfg_page(sc, address, &hdr0, 1, &pg0, sizeof(pg0)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to fetch page 0\n",
		    DEVNAME(sc));
		return (EIO);
	}

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr dev pg 0 neg_params1: 0x%02x "
	    "neg_offset: %d neg_period: 0x%02x neg_params2: 0x%02x "
	    "info: 0x%08x\n", DEVNAME(sc), pg0.neg_params1, pg0.neg_offset,
	    pg0.neg_period, pg0.neg_params2, letoh32(pg0.information));
#endif

	if (mpi_cfg_page(sc, address, &hdr1, 1, &pg1, sizeof(pg1)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to fetch page 1\n",
		    DEVNAME(sc));
		return (EIO);
	}

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr dev pg 1 req_params1: 0x%02x "
	    "req_offset: 0x%02x req_period: 0x%02x req_params2: 0x%02x "
	    "conf: 0x%08x\n", DEVNAME(sc), pg1.req_params1, pg1.req_offset,
	    pg1.req_period, pg1.req_params2, letoh32(pg1.configuration));

	pg1.req_params1 = 0;
	pg1.req_offset = offset;
	pg1.req_period = period;
	pg1.req_params2 &= ~MPI_CFG_SPI_DEV_1_REQPARAMS_WIDTH;

	if (raid || !(link->quirks & SDEV_NOSYNC)) {
		pg1.req_params2 |= MPI_CFG_SPI_DEV_1_REQPARAMS_WIDTH_WIDE;

		switch (try) {
		case 0: /* U320 */
			break;
		case 1: /* U160 */
			pg1.req_period = 0x09;
			break;
		case 2: /* U80 */
			pg1.req_period = 0x0a;
			break;
		}

		if (pg1.req_period < 0x09) {
			/* Ultra320: enable QAS & PACKETIZED */
			pg1.req_params1 |= MPI_CFG_SPI_DEV_1_REQPARAMS_QAS |
			    MPI_CFG_SPI_DEV_1_REQPARAMS_PACKETIZED;
		}
		if (pg1.req_period < 0xa) {
			/* >= Ultra160: enable dual xfers */
			pg1.req_params1 |=
			    MPI_CFG_SPI_DEV_1_REQPARAMS_DUALXFERS;
		}
	}

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr dev pg 1 req_params1: 0x%02x "
	    "req_offset: 0x%02x req_period: 0x%02x req_params2: 0x%02x "
	    "conf: 0x%08x\n", DEVNAME(sc), pg1.req_params1, pg1.req_offset,
	    pg1.req_period, pg1.req_params2, letoh32(pg1.configuration));

	if (mpi_cfg_page(sc, address, &hdr1, 0, &pg1, sizeof(pg1)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to write page 1\n",
		    DEVNAME(sc));
		return (EIO);
	}

	if (mpi_cfg_page(sc, address, &hdr1, 1, &pg1, sizeof(pg1)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to read page 1\n",
		    DEVNAME(sc));
		return (EIO);
	}

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr dev pg 1 req_params1: 0x%02x "
	    "req_offset: 0x%02x req_period: 0x%02x req_params2: 0x%02x "
	    "conf: 0x%08x\n", DEVNAME(sc), pg1.req_params1, pg1.req_offset,
	    pg1.req_period, pg1.req_params2, letoh32(pg1.configuration));

	if (mpi_inq(sc, id, raid) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to do inquiry against "
		    "target %d\n", DEVNAME(sc), link->target);
		return (EIO);
	}

	if (mpi_cfg_page(sc, address, &hdr0, 1, &pg0, sizeof(pg0)) != 0) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr unable to read page 0 after "
		    "inquiry\n", DEVNAME(sc));
		return (EIO);
	}

	DNPRINTF(MPI_D_PPR, "%s: mpi_ppr dev pg 0 neg_params1: 0x%02x "
	    "neg_offset: %d neg_period: 0x%02x neg_params2: 0x%02x "
	    "info: 0x%08x\n", DEVNAME(sc), pg0.neg_params1, pg0.neg_offset,
	    pg0.neg_period, pg0.neg_params2, letoh32(pg0.information));

	if (!(letoh32(pg0.information) & 0x07) && (try == 0)) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr U320 ppr rejected\n",
		    DEVNAME(sc));
		return (EAGAIN);
	}

	if ((((letoh32(pg0.information) >> 8) & 0xff) > 0x09) && (try == 1)) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr U160 ppr rejected\n",
		    DEVNAME(sc));
		return (EAGAIN);
	}

	if (letoh32(pg0.information) & 0x0e) {
		DNPRINTF(MPI_D_PPR, "%s: mpi_ppr ppr rejected: %0x\n",
		    DEVNAME(sc), letoh32(pg0.information));
		return (EAGAIN);
	}

	switch(pg0.neg_period) {
	case 0x08:
		period = 160;
		break;
	case 0x09:
		period = 80;
		break;
	case 0x0a:
		period = 40;
		break;
	case 0x0b:
		period = 20;
		break;
	case 0x0c:
		period = 10;
		break;
	default:
		period = 0;
		break;
	}

	printf("%s: %s %d %s at %dMHz width %dbit offset %d "
	    "QAS %d DT %d IU %d\n", DEVNAME(sc), raid ? "phys disk" : "target",
	    id, period ? "Sync" : "Async", period,
	    (pg0.neg_params2 & MPI_CFG_SPI_DEV_0_NEGPARAMS_WIDTH_WIDE) ? 16 : 8,
	    pg0.neg_offset,
	    (pg0.neg_params1 & MPI_CFG_SPI_DEV_0_NEGPARAMS_QAS) ? 1 : 0,
	    (pg0.neg_params1 & MPI_CFG_SPI_DEV_0_NEGPARAMS_DUALXFERS) ? 1 : 0,
	    (pg0.neg_params1 & MPI_CFG_SPI_DEV_0_NEGPARAMS_PACKETIZED) ? 1 : 0);

	return (0);
}

int
mpi_inq(struct mpi_softc *sc, u_int16_t target, int physdisk)
{
	struct mpi_ccb			*ccb;
	struct scsi_inquiry		inq;
	struct {
		struct mpi_msg_scsi_io		io;
		struct mpi_sge			sge;
		struct scsi_inquiry_data	inqbuf;
		struct scsi_sense_data		sense;
	} __packed			*bundle;
	struct mpi_msg_scsi_io		*io;
	struct mpi_sge			*sge;
	u_int64_t			addr;

	DNPRINTF(MPI_D_PPR, "%s: mpi_inq\n", DEVNAME(sc));

	bzero(&inq, sizeof(inq));
	inq.opcode = INQUIRY;

	ccb = mpi_get_ccb(sc);
	if (ccb == NULL)
		return (1);

	ccb->ccb_done = mpi_empty_done;

	bundle = ccb->ccb_cmd;
	io = &bundle->io;
	sge = &bundle->sge;

	io->function = physdisk ? MPI_FUNCTION_RAID_SCSI_IO_PASSTHROUGH :
	    MPI_FUNCTION_SCSI_IO_REQUEST;
	/*
	 * bus is always 0
	 * io->bus = htole16(sc->sc_bus);
	 */
	io->target_id = target;

	io->cdb_length = sizeof(inq);
	io->sense_buf_len = sizeof(struct scsi_sense_data);
	io->msg_flags = MPI_SCSIIO_SENSE_BUF_ADDR_WIDTH_64;

	io->msg_context = htole32(ccb->ccb_id);

	/*
	 * always lun 0
	 * io->lun[0] = htobe16(link->lun);
	 */

	io->direction = MPI_SCSIIO_DIR_READ;
	io->tagging = MPI_SCSIIO_ATTR_NO_DISCONNECT;

	bcopy(&inq, io->cdb, sizeof(inq));

	io->data_length = htole32(sizeof(struct scsi_inquiry_data));

	io->sense_buf_low_addr = htole32(ccb->ccb_cmd_dva +
	    ((u_int8_t *)&bundle->sense - (u_int8_t *)bundle));

	sge->sg_hdr = htole32(MPI_SGE_FL_TYPE_SIMPLE | MPI_SGE_FL_SIZE_64 |
	    MPI_SGE_FL_LAST | MPI_SGE_FL_EOB | MPI_SGE_FL_EOL |
	    (u_int32_t)sizeof(inq));

	addr = ccb->ccb_cmd_dva +
	    ((u_int8_t *)&bundle->inqbuf - (u_int8_t *)bundle);
	sge->sg_hi_addr = htole32((u_int32_t)(addr >> 32));
	sge->sg_lo_addr = htole32((u_int32_t)addr);

	if (mpi_poll(sc, ccb, 5000) != 0)
		return (1);

	if (ccb->ccb_rcb != NULL)
		mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);

	mpi_put_ccb(sc, ccb);

	return (0);
}

void
mpi_detach(struct mpi_softc *sc)
{

}

int
mpi_intr(void *arg)
{
	struct mpi_softc		*sc = arg;
	u_int32_t			reg;
	int				rv = 0;

	while ((reg = mpi_pop_reply(sc)) != 0xffffffff) {
		mpi_reply(sc, reg);
		rv = 1;
	}

	return (rv);
}

int
mpi_reply(struct mpi_softc *sc, u_int32_t reg)
{
	struct mpi_ccb			*ccb;
	struct mpi_rcb			*rcb = NULL;
	struct mpi_msg_reply		*reply = NULL;
	u_int32_t			reply_dva = 0x0;
	int				id;
	int				i;

	DNPRINTF(MPI_D_INTR, "%s: mpi_reply reg: 0x%08x\n", DEVNAME(sc), reg);

	if (reg & MPI_REPLY_QUEUE_ADDRESS) {
		bus_dmamap_sync(sc->sc_dmat,
		    MPI_DMA_MAP(sc->sc_replies), 0, PAGE_SIZE,
		    BUS_DMASYNC_POSTREAD);

		reply_dva = (reg & MPI_REPLY_QUEUE_ADDRESS_MASK) << 1;

		i = (reply_dva - (u_int32_t)MPI_DMA_DVA(sc->sc_replies)) /
		    MPI_REPLY_SIZE;
		rcb = &sc->sc_rcbs[1];
		reply = rcb->rcb_reply;

		id = letoh32(reply->msg_context);

		bus_dmamap_sync(sc->sc_dmat,
		    MPI_DMA_MAP(sc->sc_replies), 0, PAGE_SIZE,
		    BUS_DMASYNC_PREREAD);
	} else {
		switch (reg & MPI_REPLY_QUEUE_TYPE_MASK) {
		case MPI_REPLY_QUEUE_TYPE_INIT:
			id = reg & MPI_REPLY_QUEUE_CONTEXT;
			break;

		default:
			panic("%s: unsupported context reply\n",
			    DEVNAME(sc));
		}
	}

	DNPRINTF(MPI_D_INTR, "%s: mpi_reply id: %d reply: %p\n",
	    DEVNAME(sc), id, reply);

	ccb = &sc->sc_ccbs[id];

	bus_dmamap_sync(sc->sc_dmat, MPI_DMA_MAP(sc->sc_requests),
	    ccb->ccb_offset, MPI_REQUEST_SIZE,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	ccb->ccb_state = MPI_CCB_READY;
	ccb->ccb_rcb = rcb;

	ccb->ccb_done(ccb);

	return (id);
}

struct mpi_dmamem *
mpi_dmamem_alloc(struct mpi_softc *sc, size_t size)
{
	struct mpi_dmamem		*mdm;
	int				nsegs;

	mdm = malloc(sizeof(struct mpi_dmamem), M_DEVBUF, M_NOWAIT);
	if (mdm == NULL)
		return (NULL);

	bzero(mdm, sizeof(struct mpi_dmamem));
	mdm->mdm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &mdm->mdm_map) != 0)
		goto mdmfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &mdm->mdm_seg,
	    1, &nsegs, BUS_DMA_NOWAIT) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &mdm->mdm_seg, nsegs, size,
	    &mdm->mdm_kva, BUS_DMA_NOWAIT) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, mdm->mdm_map, mdm->mdm_kva, size,
	    NULL, BUS_DMA_NOWAIT) != 0)
		goto unmap;

	bzero(mdm->mdm_kva, size);

	DNPRINTF(MPI_D_MEM, "%s: mpi_dmamem_alloc size: %d mdm: %#x "
	    "map: %#x nsegs: %d segs: %#x kva: %x\n",
	    DEVNAME(sc), size, mdm->mdm_map, nsegs, mdm->mdm_seg, mdm->mdm_kva);

	return (mdm);

unmap:
	bus_dmamem_unmap(sc->sc_dmat, mdm->mdm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &mdm->mdm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, mdm->mdm_map);
mdmfree:
	free(mdm, M_DEVBUF);

	return (NULL);
}

void
mpi_dmamem_free(struct mpi_softc *sc, struct mpi_dmamem *mdm)
{
	DNPRINTF(MPI_D_MEM, "%s: mpi_dmamem_free %#x\n", DEVNAME(sc), mdm);

	bus_dmamap_unload(sc->sc_dmat, mdm->mdm_map);
	bus_dmamem_unmap(sc->sc_dmat, mdm->mdm_kva, mdm->mdm_size);
	bus_dmamem_free(sc->sc_dmat, &mdm->mdm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, mdm->mdm_map);
	free(mdm, M_DEVBUF);
}

int
mpi_alloc_ccbs(struct mpi_softc *sc)
{
	struct mpi_ccb			*ccb;
	u_int8_t			*cmd;
	int				i;

	TAILQ_INIT(&sc->sc_ccb_free);

	sc->sc_ccbs = malloc(sizeof(struct mpi_ccb) * sc->sc_maxcmds,
	    M_DEVBUF, M_WAITOK);
	if (sc->sc_ccbs == NULL) {
		printf("%s: unable to allocate ccbs\n", DEVNAME(sc));
		return (1);
	}
	bzero(sc->sc_ccbs, sizeof(struct mpi_ccb) * sc->sc_maxcmds);

	sc->sc_requests = mpi_dmamem_alloc(sc,
	    MPI_REQUEST_SIZE * sc->sc_maxcmds);
	if (sc->sc_requests == NULL) {
		printf("%s: unable to allocate ccb dmamem\n", DEVNAME(sc));
		goto free_ccbs;
	}
	cmd = MPI_DMA_KVA(sc->sc_requests);
	bzero(cmd, MPI_REQUEST_SIZE * sc->sc_maxcmds);

	for (i = 0; i < sc->sc_maxcmds; i++) {
		ccb = &sc->sc_ccbs[i];

		if (bus_dmamap_create(sc->sc_dmat, MAXPHYS,
		    sc->sc_max_sgl_len, MAXPHYS, 0, 0,
		    &ccb->ccb_dmamap) != 0) {
			printf("%s: unable to create dma map\n", DEVNAME(sc));
			goto free_maps;
		}

		ccb->ccb_sc = sc;
		ccb->ccb_id = i;
		ccb->ccb_offset = MPI_REQUEST_SIZE * i;

		ccb->ccb_cmd = &cmd[ccb->ccb_offset];
		ccb->ccb_cmd_dva = (u_int32_t)MPI_DMA_DVA(sc->sc_requests) +
		    ccb->ccb_offset;

		DNPRINTF(MPI_D_CCB, "%s: mpi_alloc_ccbs(%d) ccb: %#x map: %#x "
		    "sc: %#x id: %#x offs: %#x cmd: %#x dva: %#x\n",
		    DEVNAME(sc), i, ccb, ccb->ccb_dmamap, ccb->ccb_sc,
		    ccb->ccb_id, ccb->ccb_offset, ccb->ccb_cmd,
		    ccb->ccb_cmd_dva);

		mpi_put_ccb(sc, ccb);
	}

	return (0);

free_maps:
	while ((ccb = mpi_get_ccb(sc)) != NULL)
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);

	mpi_dmamem_free(sc, sc->sc_requests);
free_ccbs:
	free(sc->sc_ccbs, M_DEVBUF);

	return (1);
}

struct mpi_ccb *
mpi_get_ccb(struct mpi_softc *sc)
{
	struct mpi_ccb			*ccb;

	ccb = TAILQ_FIRST(&sc->sc_ccb_free);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_CCB, "%s: mpi_get_ccb == NULL\n", DEVNAME(sc));
		return (NULL);
	}

	TAILQ_REMOVE(&sc->sc_ccb_free, ccb, ccb_link);

	ccb->ccb_state = MPI_CCB_READY;

	DNPRINTF(MPI_D_CCB, "%s: mpi_get_ccb %#x\n", DEVNAME(sc), ccb);

	return (ccb);
}

void
mpi_put_ccb(struct mpi_softc *sc, struct mpi_ccb *ccb)
{
	DNPRINTF(MPI_D_CCB, "%s: mpi_put_ccb %#x\n", DEVNAME(sc), ccb);

	ccb->ccb_state = MPI_CCB_FREE;
	ccb->ccb_xs = NULL;
	ccb->ccb_done = NULL;
	bzero(ccb->ccb_cmd, MPI_REQUEST_SIZE);
	TAILQ_INSERT_TAIL(&sc->sc_ccb_free, ccb, ccb_link);
}

int
mpi_alloc_replies(struct mpi_softc *sc)
{
	DNPRINTF(MPI_D_MISC, "%s: mpi_alloc_replies\n", DEVNAME(sc));

	sc->sc_rcbs = malloc(MPI_REPLY_COUNT * sizeof(struct mpi_rcb),
	    M_DEVBUF, M_WAITOK);
	if (sc->sc_rcbs == NULL)
		return (1);

	sc->sc_replies = mpi_dmamem_alloc(sc, PAGE_SIZE);
	if (sc->sc_replies == NULL) {
		free(sc->sc_rcbs, M_DEVBUF);
		return (1);
	}

	return (0);
}

void
mpi_push_replies(struct mpi_softc *sc)
{
	struct mpi_rcb			*rcb;
	char				*kva = MPI_DMA_KVA(sc->sc_replies);
	int				i;

	bus_dmamap_sync(sc->sc_dmat, MPI_DMA_MAP(sc->sc_replies),
	    0, PAGE_SIZE, BUS_DMASYNC_PREREAD);

	for (i = 0; i < MPI_REPLY_COUNT; i++) {
		rcb = &sc->sc_rcbs[i];

		rcb->rcb_reply = kva + MPI_REPLY_SIZE * i;
		rcb->rcb_reply_dva = (u_int32_t)MPI_DMA_DVA(sc->sc_replies) +
		    MPI_REPLY_SIZE * i;
		mpi_push_reply(sc, rcb->rcb_reply_dva);
	}
}

void
mpi_start(struct mpi_softc *sc, struct mpi_ccb *ccb)
{
	DNPRINTF(MPI_D_RW, "%s: mpi_start %#x\n", DEVNAME(sc),
	    ccb->ccb_cmd_dva);

	bus_dmamap_sync(sc->sc_dmat, MPI_DMA_MAP(sc->sc_requests),
	    ccb->ccb_offset, MPI_REQUEST_SIZE,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	ccb->ccb_state = MPI_CCB_QUEUED;
	mpi_write(sc, MPI_REQ_QUEUE, ccb->ccb_cmd_dva);
}

int
mpi_complete(struct mpi_softc *sc, struct mpi_ccb *ccb, int timeout)
{
	u_int32_t			reg;
	int				id = -1;

	DNPRINTF(MPI_D_INTR, "%s: mpi_complete timeout %d\n", DEVNAME(sc),
	    timeout);

	do {
		reg = mpi_pop_reply(sc);
		if (reg == 0xffffffff) {
			if (timeout-- == 0)
				return (1);

			delay(1000);
			continue;
		}

		id = mpi_reply(sc, reg);

	} while (ccb->ccb_id != id);

	return (0);
}

int
mpi_poll(struct mpi_softc *sc, struct mpi_ccb *ccb, int timeout)
{
	int				error;
	int				s;

	DNPRINTF(MPI_D_CMD, "%s: mpi_poll\n", DEVNAME(sc));

	s = splbio();
	mpi_start(sc, ccb);
	error = mpi_complete(sc, ccb, timeout);
	splx(s);

	return (error);
}

int
mpi_scsi_cmd(struct scsi_xfer *xs)
{
	struct scsi_link		*link = xs->sc_link;
	struct mpi_softc		*sc = link->adapter_softc;
	struct mpi_ccb			*ccb;
	struct mpi_ccb_bundle		*mcb;
	struct mpi_msg_scsi_io		*io;
	int				s;

	DNPRINTF(MPI_D_CMD, "%s: mpi_scsi_cmd\n", DEVNAME(sc));

	if (xs->cmdlen > MPI_CDB_LEN) {
		DNPRINTF(MPI_D_CMD, "%s: CBD too big %d\n",
		    DEVNAME(sc), xs->cmdlen);
		bzero(&xs->sense, sizeof(xs->sense));
		xs->sense.error_code = SSD_ERRCODE_VALID | 0x70;
		xs->sense.flags = SKEY_ILLEGAL_REQUEST;
		xs->sense.add_sense_code = 0x20;
		xs->error = XS_SENSE;
		s = splbio();
		scsi_done(xs);
		splx(s);
		return (COMPLETE);
	}

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		xs->error = XS_DRIVER_STUFFUP;
		s = splbio();
		scsi_done(xs);
		splx(s);
		return (COMPLETE);
	}
	DNPRINTF(MPI_D_CMD, "%s: ccb_id: %d xs->flags: 0x%x\n",
	    DEVNAME(sc), ccb->ccb_id, xs->flags);

	ccb->ccb_xs = xs;
	ccb->ccb_done = mpi_scsi_cmd_done;

	mcb = ccb->ccb_cmd;
	io = &mcb->mcb_io;

	io->function = MPI_FUNCTION_SCSI_IO_REQUEST;
	/*
	 * bus is always 0
	 * io->bus = htole16(sc->sc_bus);
	 */
	io->target_id = link->target;

	io->cdb_length = xs->cmdlen;
	io->sense_buf_len = sizeof(xs->sense);
	io->msg_flags = MPI_SCSIIO_SENSE_BUF_ADDR_WIDTH_64;

	io->msg_context = htole32(ccb->ccb_id);

	io->lun[0] = htobe16(link->lun);

	switch (xs->flags & (SCSI_DATA_IN | SCSI_DATA_OUT)) {
	case SCSI_DATA_IN:
		io->direction = MPI_SCSIIO_DIR_READ;
		break;
	case SCSI_DATA_OUT:
		io->direction = MPI_SCSIIO_DIR_WRITE;
		break;
	default:
		io->direction = MPI_SCSIIO_DIR_NONE;
		break;
	}

	if (link->quirks & SDEV_NOTAGS)
		io->tagging = MPI_SCSIIO_ATTR_UNTAGGED;
	else 
		io->tagging = MPI_SCSIIO_ATTR_SIMPLE_Q;

	bcopy(xs->cmd, io->cdb, xs->cmdlen);

	io->data_length = htole32(xs->datalen);

	io->sense_buf_low_addr = htole32(ccb->ccb_cmd_dva +
	    ((u_int8_t *)&mcb->mcb_sense - (u_int8_t *)mcb));

	if (mpi_load_xs(ccb) != 0) {
		xs->error = XS_DRIVER_STUFFUP;
		s = splbio();
		mpi_put_ccb(sc, ccb);
		scsi_done(xs);
		splx(s);
		return (COMPLETE);
	}

	timeout_set(&xs->stimeout, mpi_timeout_xs, ccb);

	if (xs->flags & SCSI_POLL) {
		if (mpi_poll(sc, ccb, xs->timeout) != 0)
			xs->error = XS_DRIVER_STUFFUP;
		return (COMPLETE);
	}

	s = splbio();
	mpi_start(sc, ccb);
	splx(s);
	return (SUCCESSFULLY_QUEUED);
}

void
mpi_scsi_cmd_done(struct mpi_ccb *ccb)
{
	struct mpi_softc		*sc = ccb->ccb_sc;
	struct scsi_xfer		*xs = ccb->ccb_xs;
	struct mpi_ccb_bundle		*mcb = ccb->ccb_cmd;
	bus_dmamap_t			dmap = ccb->ccb_dmamap;
	struct mpi_msg_scsi_io_error	*sie;

	if (xs->datalen != 0) {
		bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
		    (xs->flags & SCSI_DATA_IN) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);

		bus_dmamap_unload(sc->sc_dmat, dmap);
	}

	/* timeout_del */
	xs->error = XS_NOERROR;
	xs->resid = 0;
	xs->flags |= ITSDONE;

	if (ccb->ccb_rcb == NULL) {
		/* no scsi error, we're ok so drop out early */
		xs->status = SCSI_OK;
		mpi_put_ccb(sc, ccb);
		scsi_done(xs);
		return;
	}

	sie = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPI_D_CMD, "%s: mpi_scsi_cmd_done xs cmd: 0x%02x len: %d "
	    "flags 0x%x\n", DEVNAME(sc), xs->cmd->opcode, xs->datalen,
	    xs->flags);
	DNPRINTF(MPI_D_CMD, "%s:  target_id: %d bus: %d msg_length: %d "
	    "function: 0x%02x\n", DEVNAME(sc), sie->target_id, sie->bus,
	    sie->msg_length, sie->function);
	DNPRINTF(MPI_D_CMD, "%s:  cdb_length: %d sense_buf_length: %d "
	    "msg_flags: 0x%02x\n", DEVNAME(sc), sie->cdb_length,
	    sie->sense_buf_len, sie->msg_flags);
	DNPRINTF(MPI_D_CMD, "%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(sie->msg_context));
	DNPRINTF(MPI_D_CMD, "%s:  scsi_status: 0x%02x scsi_state: 0x%02x "
	    "ioc_status: 0x%04x\n", DEVNAME(sc), sie->scsi_status,
	    sie->scsi_state, letoh16(sie->ioc_status));
	DNPRINTF(MPI_D_CMD, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(sie->ioc_loginfo));
	DNPRINTF(MPI_D_CMD, "%s:  transfer_count: %d\n", DEVNAME(sc),
	    letoh32(sie->transfer_count));
	DNPRINTF(MPI_D_CMD, "%s:  sense_count: %d\n", DEVNAME(sc),
	    letoh32(sie->sense_count));
	DNPRINTF(MPI_D_CMD, "%s:  response_info: 0x%08x\n", DEVNAME(sc),
	    letoh32(sie->response_info));
	DNPRINTF(MPI_D_CMD, "%s:  tag: 0x%04x\n", DEVNAME(sc),
	    letoh16(sie->tag));

	xs->status = sie->scsi_status;
	switch (letoh16(sie->ioc_status)) {
	case MPI_IOCSTATUS_SCSI_DATA_UNDERRUN:
		xs->resid = xs->datalen - letoh32(sie->transfer_count);
		if (sie->scsi_state & MPI_SCSIIO_ERR_STATE_NO_SCSI_STATUS) {
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		/* FALLTHROUGH */
	case MPI_IOCSTATUS_SUCCESS:
	case MPI_IOCSTATUS_SCSI_RECOVERED_ERROR:
		switch (xs->status) {
		case SCSI_OK:
			xs->resid = 0;
			break;

		case SCSI_CHECK:
			xs->error = XS_SENSE;
			break;

		case SCSI_BUSY:
		case SCSI_QUEUE_FULL:
			xs->error = XS_BUSY;
			break;

		default:
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		break;

	case MPI_IOCSTATUS_BUSY:
	case MPI_IOCSTATUS_INSUFFICIENT_RESOURCES:
		xs->error = XS_BUSY;
		break;

	case MPI_IOCSTATUS_SCSI_INVALID_BUS:
	case MPI_IOCSTATUS_SCSI_INVALID_TARGETID:
	case MPI_IOCSTATUS_SCSI_DEVICE_NOT_THERE:
		xs->error = XS_SELTIMEOUT;
		break;

	default:
		xs->error = XS_DRIVER_STUFFUP;
		break;
	}

	if (sie->scsi_state & MPI_SCSIIO_ERR_STATE_AUTOSENSE_VALID)
		bcopy(&mcb->mcb_sense, &xs->sense, sizeof(xs->sense));

	DNPRINTF(MPI_D_CMD, "%s:  xs err: 0x%02x status: %d\n", DEVNAME(sc),
	    xs->error, xs->status);

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	mpi_put_ccb(sc, ccb);
	scsi_done(xs);
}

void
mpi_timeout_xs(void *arg)
{
	/* XXX */
}

int
mpi_load_xs(struct mpi_ccb *ccb)
{
	struct mpi_softc		*sc = ccb->ccb_sc;
	struct scsi_xfer		*xs = ccb->ccb_xs;
	struct mpi_ccb_bundle		*mcb = ccb->ccb_cmd;
	struct mpi_msg_scsi_io		*io = &mcb->mcb_io;
	struct mpi_sge			*sge, *nsge = &mcb->mcb_sgl[0];
	struct mpi_sge			*ce = NULL, *nce;
	u_int64_t			ce_dva;
	bus_dmamap_t			dmap = ccb->ccb_dmamap;
	u_int32_t			addr, flags;
	int				i, error;

	if (xs->datalen == 0) {
		nsge->sg_hdr = htole32(MPI_SGE_FL_TYPE_SIMPLE |
		    MPI_SGE_FL_LAST | MPI_SGE_FL_EOB | MPI_SGE_FL_EOL);
		return (0);
	}

	error = bus_dmamap_load(sc->sc_dmat, dmap,
	    xs->data, xs->datalen, NULL,
	    (xs->flags & SCSI_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	if (error) {
		printf("%s: error %d loading dmamap\n", DEVNAME(sc), error);
		return (1);
	}

	flags = MPI_SGE_FL_TYPE_SIMPLE | MPI_SGE_FL_SIZE_64;
	if (xs->flags & SCSI_DATA_OUT)
		flags |= MPI_SGE_FL_DIR_OUT;

	if (dmap->dm_nsegs > sc->sc_first_sgl_len) {
		ce = &mcb->mcb_sgl[sc->sc_first_sgl_len - 1];
		io->chain_offset = ((u_int8_t *)ce - (u_int8_t *)io) / 4;
	}

	for (i = 0; i < dmap->dm_nsegs; i++) {

		if (nsge == ce) {
			nsge++;
			sge->sg_hdr |= htole32(MPI_SGE_FL_LAST);

			DNPRINTF(MPI_D_DMA, "%s:   - 0x%08x 0x%08x 0x%08x\n",
			    DEVNAME(sc), sge->sg_hdr,
			    sge->sg_hi_addr, sge->sg_lo_addr);

			if ((dmap->dm_nsegs - i) > sc->sc_chain_len) {
				nce = &nsge[sc->sc_chain_len - 1];
				addr = ((u_int8_t *)nce - (u_int8_t *)nsge) / 4;
				addr = addr << 16 |
				    sizeof(struct mpi_sge) * sc->sc_chain_len;
			} else {
				nce = NULL;
				addr = sizeof(struct mpi_sge) *
				    (dmap->dm_nsegs - i);
			}

			ce->sg_hdr = htole32(MPI_SGE_FL_TYPE_CHAIN |
			    MPI_SGE_FL_SIZE_64 | addr);

			ce_dva = ccb->ccb_cmd_dva +
			    ((u_int8_t *)nsge - (u_int8_t *)mcb);

			addr = (u_int32_t)(ce_dva >> 32);
			ce->sg_hi_addr = htole32(addr);
			addr = (u_int32_t)ce_dva;
			ce->sg_lo_addr = htole32(addr);

			DNPRINTF(MPI_D_DMA, "%s:  ce: 0x%08x 0x%08x 0x%08x\n",
			    DEVNAME(sc), ce->sg_hdr, ce->sg_hi_addr,
			    ce->sg_lo_addr);

			ce = nce;
		}

		DNPRINTF(MPI_D_DMA, "%s:  %d: %d 0x%016llx\n", DEVNAME(sc),
		    i, dmap->dm_segs[i].ds_len,
		    (u_int64_t)dmap->dm_segs[i].ds_addr);

		sge = nsge;

		sge->sg_hdr = htole32(flags | dmap->dm_segs[i].ds_len);
		addr = (u_int32_t)((u_int64_t)dmap->dm_segs[i].ds_addr >> 32);
		sge->sg_hi_addr = htole32(addr);
		addr = (u_int32_t)dmap->dm_segs[i].ds_addr;
		sge->sg_lo_addr = htole32(addr);

		DNPRINTF(MPI_D_DMA, "%s:  %d: 0x%08x 0x%08x 0x%08x\n",
		    DEVNAME(sc), i, sge->sg_hdr, sge->sg_hi_addr,
		    sge->sg_lo_addr);

		nsge = sge + 1;
	}

	/* terminate list */
	sge->sg_hdr |= htole32(MPI_SGE_FL_LAST | MPI_SGE_FL_EOB |
	    MPI_SGE_FL_EOL);

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->flags & SCSI_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

	return (0);
}

void
mpi_minphys(struct buf *bp)
{
	/* XXX */
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
	minphys(bp);
}

int
mpi_scsi_ioctl(struct scsi_link *a, u_long b, caddr_t c, int d, struct proc *e)
{
	return (0);
}

u_int32_t
mpi_read(struct mpi_softc *sc, bus_size_t r)
{
	u_int32_t			rv;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	rv = bus_space_read_4(sc->sc_iot, sc->sc_ioh, r);

	DNPRINTF(MPI_D_RW, "%s: mpi_read %#x %#x\n", DEVNAME(sc), r, rv);

	return (rv);
}

void
mpi_write(struct mpi_softc *sc, bus_size_t r, u_int32_t v)
{
	DNPRINTF(MPI_D_RW, "%s: mpi_write %#x %#x\n", DEVNAME(sc), r, v);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

int
mpi_wait_eq(struct mpi_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{
	int				i;

	DNPRINTF(MPI_D_RW, "%s: mpi_wait_eq %#x %#x %#x\n", DEVNAME(sc), r,
	    mask, target);

	for (i = 0; i < 10000; i++) {
		if ((mpi_read(sc, r) & mask) == target)
			return (0);
		delay(1000);
	}

	return (1);
}

int
mpi_wait_ne(struct mpi_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{
	int				i;

	DNPRINTF(MPI_D_RW, "%s: mpi_wait_ne %#x %#x %#x\n", DEVNAME(sc), r,
	    mask, target);

	for (i = 0; i < 10000; i++) {
		if ((mpi_read(sc, r) & mask) != target)
			return (0);
		delay(1000);
	}

	return (1);
}

int
mpi_init(struct mpi_softc *sc)
{
	u_int32_t			db;
	int				i;

	/* spin until the IOC leaves the RESET state */
	if (mpi_wait_ne(sc, MPI_DOORBELL, MPI_DOORBELL_STATE,
	    MPI_DOORBELL_STATE_RESET) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_init timeout waiting to leave "
		    "reset state\n", DEVNAME(sc));
		return (1);
	}

	/* check current ownership */
	db = mpi_read_db(sc);
	if ((db & MPI_DOORBELL_WHOINIT) == MPI_DOORBELL_WHOINIT_PCIPEER) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_init initialised by pci peer\n",
		    DEVNAME(sc));
		return (0);
	}

	for (i = 0; i < 5; i++) {
		switch (db & MPI_DOORBELL_STATE) {
		case MPI_DOORBELL_STATE_READY:
			DNPRINTF(MPI_D_MISC, "%s: mpi_init ioc is ready\n",
			    DEVNAME(sc));
			return (0);

		case MPI_DOORBELL_STATE_OPER:
		case MPI_DOORBELL_STATE_FAULT:
			DNPRINTF(MPI_D_MISC, "%s: mpi_init ioc is being "
			    "reset\n" , DEVNAME(sc));
			if (mpi_reset_soft(sc) != 0)
				mpi_reset_hard(sc);
			break;

		case MPI_DOORBELL_STATE_RESET:
			DNPRINTF(MPI_D_MISC, "%s: mpi_init waiting to come "
			    "out of reset\n", DEVNAME(sc));
			if (mpi_wait_ne(sc, MPI_DOORBELL, MPI_DOORBELL_STATE,
			    MPI_DOORBELL_STATE_RESET) != 0)
				return (1);
			break;
		}
		db = mpi_read_db(sc);
	}

	return (1);
}

int
mpi_reset_soft(struct mpi_softc *sc)
{
	DNPRINTF(MPI_D_MISC, "%s: mpi_reset_soft\n", DEVNAME(sc));

	if (mpi_read_db(sc) & MPI_DOORBELL_INUSE)
		return (1);

	mpi_write_db(sc,
	    MPI_DOORBELL_FUNCTION(MPI_FUNCTION_IOC_MESSAGE_UNIT_RESET));
	if (mpi_wait_eq(sc, MPI_INTR_STATUS,
	    MPI_INTR_STATUS_IOCDOORBELL, 0) != 0)
		return (1);

	if (mpi_wait_eq(sc, MPI_DOORBELL, MPI_DOORBELL_STATE,
	    MPI_DOORBELL_STATE_READY) != 0)
		return (1);

	return (0);
}

int
mpi_reset_hard(struct mpi_softc *sc)
{
	DNPRINTF(MPI_D_MISC, "%s: mpi_reset_hard\n", DEVNAME(sc));

	/* enable diagnostic register */
	mpi_write(sc, MPI_WRITESEQ, 0xff);
	mpi_write(sc, MPI_WRITESEQ, MPI_WRITESEQ_1);
	mpi_write(sc, MPI_WRITESEQ, MPI_WRITESEQ_2);
	mpi_write(sc, MPI_WRITESEQ, MPI_WRITESEQ_3);
	mpi_write(sc, MPI_WRITESEQ, MPI_WRITESEQ_4);
	mpi_write(sc, MPI_WRITESEQ, MPI_WRITESEQ_5);

	/* reset ioc */
	mpi_write(sc, MPI_HOSTDIAG, MPI_HOSTDIAG_RESET_ADAPTER);

	delay(10000);

	/* disable diagnostic register */
	mpi_write(sc, MPI_WRITESEQ, 0xff);

	/* restore pci bits? */

	/* firmware bits? */
	return (0);
}

int
mpi_handshake_send(struct mpi_softc *sc, void *buf, size_t dwords)
{
	u_int32_t				*query = buf;
	int					i;

	/* make sure the doorbell is not in use. */
	if (mpi_read_db(sc) & MPI_DOORBELL_INUSE)
		return (1);

	/* clear pending doorbell interrupts */
	if (mpi_read_intr(sc) & MPI_INTR_STATUS_DOORBELL)
		mpi_write_intr(sc, 0);

	/*
	 * first write the doorbell with the handshake function and the
	 * dword count.
	 */
	mpi_write_db(sc, MPI_DOORBELL_FUNCTION(MPI_FUNCTION_HANDSHAKE) |
	    MPI_DOORBELL_DWORDS(dwords));

	/*
	 * the doorbell used bit will be set because a doorbell function has
	 * started. Wait for the interrupt and then ack it.
	 */
	if (mpi_wait_db_int(sc) != 0)
		return (1);
	mpi_write_intr(sc, 0);

	/* poll for the acknowledgement. */
	if (mpi_wait_db_ack(sc) != 0)
		return (1);

	/* write the query through the doorbell. */
	for (i = 0; i < dwords; i++) {
		mpi_write_db(sc, htole32(query[i]));
		if (mpi_wait_db_ack(sc) != 0)
			return (1);
	}

	return (0);
}

int
mpi_handshake_recv_dword(struct mpi_softc *sc, u_int32_t *dword)
{
	u_int16_t				*words = (u_int16_t *)dword;
	int					i;

	for (i = 0; i < 2; i++) {
		if (mpi_wait_db_int(sc) != 0)
			return (1);
		words[i] = letoh16(mpi_read_db(sc) & MPI_DOORBELL_DATA_MASK);
		mpi_write_intr(sc, 0);
	}

	return (0);
}

int
mpi_handshake_recv(struct mpi_softc *sc, void *buf, size_t dwords)
{
	struct mpi_msg_reply			*reply = buf;
	u_int32_t				*dbuf = buf, dummy;
	int					i;

	/* get the first dword so we can read the length out of the header. */
	if (mpi_handshake_recv_dword(sc, &dbuf[0]) != 0)
		return (1);

	DNPRINTF(MPI_D_CMD, "%s: mpi_handshake_recv dwords: %d reply: %d\n",
	    DEVNAME(sc), dwords, reply->msg_length);

	/*
	 * the total length, in dwords, is in the message length field of the
	 * reply header.
	 */
	for (i = 1; i < MIN(dwords, reply->msg_length); i++) {
		if (mpi_handshake_recv_dword(sc, &dbuf[i]) != 0)
			return (1);
	}

	/* if there's extra stuff to come off the ioc, discard it */
	while (i++ < reply->msg_length) {
		if (mpi_handshake_recv_dword(sc, &dummy) != 0)
			return (1);
		DNPRINTF(MPI_D_CMD, "%s: mpi_handshake_recv dummy read: "
		    "0x%08x\n", DEVNAME(sc), dummy);
	}

	/* wait for the doorbell used bit to be reset and clear the intr */
	if (mpi_wait_db_int(sc) != 0)
		return (1);
	mpi_write_intr(sc, 0);

	return (0);
}

void
mpi_empty_done(struct mpi_ccb *ccb)
{
	/* nothing to do */
}

int
mpi_iocfacts(struct mpi_softc *sc)
{
	struct mpi_msg_iocfacts_request		ifq;
	struct mpi_msg_iocfacts_reply		ifp;

	DNPRINTF(MPI_D_MISC, "%s: mpi_iocfacts\n", DEVNAME(sc));

	bzero(&ifq, sizeof(ifq));
	bzero(&ifp, sizeof(ifp));

	ifq.function = MPI_FUNCTION_IOC_FACTS;
	ifq.chain_offset = 0;
	ifq.msg_flags = 0;
	ifq.msg_context = htole32(0xdeadbeef);

	if (mpi_handshake_send(sc, &ifq, dwordsof(ifq)) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_iocfacts send failed\n",
		    DEVNAME(sc));
		return (1);
	}

	if (mpi_handshake_recv(sc, &ifp, dwordsof(ifp)) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_iocfacts recv failed\n",
		    DEVNAME(sc));
		return (1);
	}

	DNPRINTF(MPI_D_MISC, "%s:  func: 0x%02x len: %d msgver: %d.%d\n",
	    DEVNAME(sc), ifp.function, ifp.msg_length,
	    ifp.msg_version_maj, ifp.msg_version_min);
	DNPRINTF(MPI_D_MISC, "%s:  msgflags: 0x%02x iocnumber: 0x%02x "
	    "hdrver: %d.%d\n", DEVNAME(sc), ifp.msg_flags,
	    ifp.ioc_number, ifp.header_version_maj,
	    ifp.header_version_min);
	DNPRINTF(MPI_D_MISC, "%s:  message context: 0x%08x\n", DEVNAME(sc),
	    letoh32(ifp.msg_context));
	DNPRINTF(MPI_D_MISC, "%s:  iocstatus: 0x%04x ioexcept: 0x%04x\n",
	    DEVNAME(sc), letoh16(ifp.ioc_status),
	    letoh16(ifp.ioc_exceptions));
	DNPRINTF(MPI_D_MISC, "%s:  iocloginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(ifp.ioc_loginfo));
	DNPRINTF(MPI_D_MISC, "%s:  flags: 0x%02x blocksize: %d whoinit: 0x%02x "
	    "maxchdepth: %d\n", DEVNAME(sc), ifp.flags,
	    ifp.block_size, ifp.whoinit, ifp.max_chain_depth);
	DNPRINTF(MPI_D_MISC, "%s:  reqfrsize: %d replyqdepth: %d\n",
	    DEVNAME(sc), letoh16(ifp.request_frame_size),
	    letoh16(ifp.reply_queue_depth));
	DNPRINTF(MPI_D_MISC, "%s:  productid: 0x%04x\n", DEVNAME(sc),
	    letoh16(ifp.product_id));
	DNPRINTF(MPI_D_MISC, "%s:  hostmfahiaddr: 0x%08x\n", DEVNAME(sc),
	    letoh32(ifp.current_host_mfa_hi_addr));
	DNPRINTF(MPI_D_MISC, "%s:  event_state: 0x%02x number_of_ports: %d "
	    "global_credits: %d\n",
	    DEVNAME(sc), ifp.event_state, ifp.number_of_ports,
	    letoh16(ifp.global_credits));
	DNPRINTF(MPI_D_MISC, "%s:  sensebufhiaddr: 0x%08x\n", DEVNAME(sc),
	    letoh32(ifp.current_sense_buffer_hi_addr));
	DNPRINTF(MPI_D_MISC, "%s:  maxbus: %d maxdev: %d replyfrsize: %d\n",
	    DEVNAME(sc), ifp.max_buses, ifp.max_devices,
	    letoh16(ifp.current_reply_frame_size));
	DNPRINTF(MPI_D_MISC, "%s:  fw_image_size: %d\n", DEVNAME(sc),
	    letoh32(ifp.fw_image_size));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_capabilities: 0x%08x\n", DEVNAME(sc),
	    letoh32(ifp.ioc_capabilities));
	DNPRINTF(MPI_D_MISC, "%s:  fw_version: %d.%d fw_version_unit: 0x%02x "
	    "fw_version_dev: 0x%02x\n", DEVNAME(sc),
	    ifp.fw_version_maj, ifp.fw_version_min,
	    ifp.fw_version_unit, ifp.fw_version_dev);
	DNPRINTF(MPI_D_MISC, "%s:  hi_priority_queue_depth: 0x%04x\n",
	    DEVNAME(sc), letoh16(ifp.hi_priority_queue_depth));
	DNPRINTF(MPI_D_MISC, "%s:  host_page_buffer_sge: hdr: 0x%08x "
	    "addr 0x%08x %08x\n", DEVNAME(sc),
	    letoh32(ifp.host_page_buffer_sge.sg_hdr),
	    letoh32(ifp.host_page_buffer_sge.sg_hi_addr),
	    letoh32(ifp.host_page_buffer_sge.sg_lo_addr));

	sc->sc_maxcmds = letoh16(ifp.global_credits);
	sc->sc_maxchdepth = ifp.max_chain_depth;
	sc->sc_ioc_number = ifp.ioc_number;
	if (sc->sc_flags & MPI_F_VMWARE)
		sc->sc_buswidth = 16;
	else
		sc->sc_buswidth =
		    (ifp.max_devices == 0) ? 256 : ifp.max_devices;
	if (ifp.flags & MPI_IOCFACTS_FLAGS_FW_DOWNLOAD_BOOT)
		sc->sc_fw_len = letoh32(ifp.fw_image_size);

	/*
	 * you can fit sg elements on the end of the io cmd if they fit in the
	 * request frame size.
	 */
	sc->sc_first_sgl_len = ((letoh16(ifp.request_frame_size) * 4) -
	    sizeof(struct mpi_msg_scsi_io)) / sizeof(struct mpi_sge);
	DNPRINTF(MPI_D_MISC, "%s:   first sgl len: %d\n", DEVNAME(sc),
	    sc->sc_first_sgl_len);

	sc->sc_chain_len = (letoh16(ifp.request_frame_size) * 4) /
	    sizeof(struct mpi_sge);
	DNPRINTF(MPI_D_MISC, "%s:   chain len: %d\n", DEVNAME(sc),
	    sc->sc_chain_len);

	/* the sgl tailing the io cmd loses an entry to the chain element. */
	sc->sc_max_sgl_len = MPI_MAX_SGL - 1;
	/* the sgl chains lose an entry for each chain element */
	sc->sc_max_sgl_len -= (MPI_MAX_SGL - sc->sc_first_sgl_len) /
	    sc->sc_chain_len;
	DNPRINTF(MPI_D_MISC, "%s:   max sgl len: %d\n", DEVNAME(sc),
	    sc->sc_max_sgl_len);

	/* XXX we're ignoring the max chain depth */

	return (0);
}

int
mpi_iocinit(struct mpi_softc *sc)
{
	struct mpi_msg_iocinit_request		iiq;
	struct mpi_msg_iocinit_reply		iip;
	u_int32_t				hi_addr;

	DNPRINTF(MPI_D_MISC, "%s: mpi_iocinit\n", DEVNAME(sc));

	bzero(&iiq, sizeof(iiq));
	bzero(&iip, sizeof(iip));

	iiq.function = MPI_FUNCTION_IOC_INIT;
	iiq.whoinit = MPI_WHOINIT_HOST_DRIVER;

	iiq.max_devices = (sc->sc_buswidth == 256) ? 0 : sc->sc_buswidth;
	iiq.max_buses = 1;

	iiq.msg_context = htole32(0xd00fd00f);

	iiq.reply_frame_size = htole16(MPI_REPLY_SIZE);

	hi_addr = (u_int32_t)((u_int64_t)MPI_DMA_DVA(sc->sc_requests) >> 32);
	iiq.host_mfa_hi_addr = htole32(hi_addr);
	iiq.sense_buffer_hi_addr = htole32(hi_addr);

	hi_addr = (u_int32_t)((u_int64_t)MPI_DMA_DVA(sc->sc_replies) >> 32);
	iiq.reply_fifo_host_signalling_addr = htole32(hi_addr);

	iiq.msg_version_maj = 0x01;
	iiq.msg_version_min = 0x02;

	iiq.hdr_version_unit = 0x0d;
	iiq.hdr_version_dev = 0x00;

	if (mpi_handshake_send(sc, &iiq, dwordsof(iiq)) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_iocinit send failed\n",
		    DEVNAME(sc));
		return (1);
	}

	if (mpi_handshake_recv(sc, &iip, dwordsof(iip)) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_iocinit recv failed\n",
		    DEVNAME(sc));
		return (1);
	}

	DNPRINTF(MPI_D_MISC, "%s:  function: 0x%02x msg_length: %d "
	    "whoinit: 0x%02x\n", DEVNAME(sc), iip.function,
	    iip.msg_length, iip.whoinit);
	DNPRINTF(MPI_D_MISC, "%s:  msg_flags: 0x%02x max_buses: %d "
	    "max_devices: %d flags: 0x%02x\n", DEVNAME(sc), iip.msg_flags,
	    iip.max_buses, iip.max_devices, iip.flags);
	DNPRINTF(MPI_D_MISC, "%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(iip.msg_context));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    letoh16(iip.ioc_status));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(iip.ioc_loginfo));

	return (0);
}

int
mpi_portfacts(struct mpi_softc *sc)
{
	struct mpi_ccb				*ccb;
	struct mpi_msg_portfacts_request	*pfq;
	volatile struct mpi_msg_portfacts_reply	*pfp;
	int					s, rv = 1;

	DNPRINTF(MPI_D_MISC, "%s: mpi_portfacts\n", DEVNAME(sc));

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_portfacts ccb_get\n",
		    DEVNAME(sc));
		return (rv);
	}

	ccb->ccb_done = mpi_empty_done;
	pfq = ccb->ccb_cmd;

	pfq->function = MPI_FUNCTION_PORT_FACTS;
	pfq->chain_offset = 0;
	pfq->msg_flags = 0;
	pfq->port_number = 0;
	pfq->msg_context = htole32(ccb->ccb_id);

	if (mpi_poll(sc, ccb, 50000) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_portfacts poll\n", DEVNAME(sc));
		goto err;
	}

	if (ccb->ccb_rcb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: empty portfacts reply\n",
		    DEVNAME(sc));
		goto err;
	}
	pfp = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPI_D_MISC, "%s:  function: 0x%02x msg_length: %d\n",
	    DEVNAME(sc), pfp->function, pfp->msg_length);
	DNPRINTF(MPI_D_MISC, "%s:  msg_flags: 0x%02x port_number: %d\n",
	    DEVNAME(sc), pfp->msg_flags, pfp->port_number);
	DNPRINTF(MPI_D_MISC, "%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(pfp->msg_context));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    letoh16(pfp->ioc_status));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(pfp->ioc_loginfo));
	DNPRINTF(MPI_D_MISC, "%s:  max_devices: %d port_type: 0x%02x\n",
	    DEVNAME(sc), letoh16(pfp->max_devices), pfp->port_type);
	DNPRINTF(MPI_D_MISC, "%s:  protocol_flags: 0x%04x port_scsi_id: %d\n",
	    DEVNAME(sc), letoh16(pfp->protocol_flags),
	    letoh16(pfp->port_scsi_id));
	DNPRINTF(MPI_D_MISC, "%s:  max_persistent_ids: %d "
	    "max_posted_cmd_buffers: %d\n", DEVNAME(sc),
	    letoh16(pfp->max_persistent_ids),
	    letoh16(pfp->max_posted_cmd_buffers));
	DNPRINTF(MPI_D_MISC, "%s:  max_lan_buckets: %d\n", DEVNAME(sc),
	    letoh16(pfp->max_lan_buckets));

	sc->sc_porttype = pfp->port_type;
	sc->sc_target = letoh16(pfp->port_scsi_id);

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	rv = 0;
err:
	mpi_put_ccb(sc, ccb);

	return (rv);
}

int
mpi_eventnotify(struct mpi_softc *sc)
{
	struct mpi_ccb				*ccb;
	struct mpi_msg_event_request		*enq;
	int					s;

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_eventnotify ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	ccb->ccb_done = mpi_eventnotify_done;
	enq = ccb->ccb_cmd;

	enq->function = MPI_FUNCTION_EVENT_NOTIFICATION;
	enq->chain_offset = 0;
	enq->event_switch = MPI_EVENT_SWITCH_ON;
	enq->msg_context = htole32(ccb->ccb_id);

	mpi_start(sc, ccb);
	return (0);
}

void
mpi_eventnotify_done(struct mpi_ccb *ccb)
{
	struct mpi_softc			*sc = ccb->ccb_sc;
	struct mpi_msg_event_reply		*enp = ccb->ccb_rcb->rcb_reply;
	u_int32_t				*data;
	int					i;

	printf("%s: %s\n", DEVNAME(sc), __func__);

	printf("%s:  function: 0x%02x msg_length: %d data_length: %d\n",
	    DEVNAME(sc), enp->function, enp->msg_length,
	    letoh16(enp->data_length));

	printf("%s:  ack_required: %d msg_flags 0x%02x\n", DEVNAME(sc),
	    enp->msg_flags, enp->msg_flags);

	printf("%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(enp->msg_context));

	printf("%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    letoh16(enp->ioc_status));

	printf("%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(enp->ioc_loginfo));

	data = ccb->ccb_rcb->rcb_reply;
	data += dwordsof(struct mpi_msg_event_reply);
	for (i = 0; i < letoh16(enp->data_length); i++) {
		printf("%s:  data[%d]: 0x%08x\n", DEVNAME(sc), i, data[i]);
	}
}

int
mpi_portenable(struct mpi_softc *sc)
{
	struct mpi_ccb				*ccb;
	struct mpi_msg_portenable_request	*peq;
	struct mpi_msg_portenable_repy		*pep;
	int					s;

	DNPRINTF(MPI_D_MISC, "%s: mpi_portenable\n", DEVNAME(sc));

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_portenable ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	ccb->ccb_done = mpi_empty_done;
	peq = ccb->ccb_cmd;

	peq->function = MPI_FUNCTION_PORT_ENABLE;
	peq->port_number = 0;
	peq->msg_context = htole32(ccb->ccb_id);

	if (mpi_poll(sc, ccb, 50000) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_portenable poll\n", DEVNAME(sc));
		return (1);
	}

	if (ccb->ccb_rcb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: empty portenable reply\n",
		    DEVNAME(sc));
		return (1);
	}
	pep = ccb->ccb_rcb->rcb_reply;

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	mpi_put_ccb(sc, ccb);

	return (0);
}

int
mpi_fwupload(struct mpi_softc *sc)
{
	struct mpi_ccb				*ccb;
	struct {
		struct mpi_msg_fwupload_request		req;
		struct mpi_sge				sge;
	} __packed				*bundle;
	struct mpi_msg_fwupload_reply		*upp;
	u_int64_t				addr;
	int					s;
	int					rv = 0;

	if (sc->sc_fw_len == 0)
		return (0);

	DNPRINTF(MPI_D_MISC, "%s: mpi_fwupload\n", DEVNAME(sc));

	sc->sc_fw = mpi_dmamem_alloc(sc, sc->sc_fw_len);
	if (sc->sc_fw == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_fwupload unable to allocate %d\n",
		    DEVNAME(sc), sc->sc_fw_len);
		return (1);
	}

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_fwupload ccb_get\n",
		    DEVNAME(sc));
		goto err;
	}

	ccb->ccb_done = mpi_empty_done;
	bundle = ccb->ccb_cmd;

	bundle->req.function = MPI_FUNCTION_FW_UPLOAD;
	bundle->req.msg_context = htole32(ccb->ccb_id);

	bundle->req.image_type = MPI_FWUPLOAD_IMAGETYPE_IOC_FW;

	bundle->req.tce.details_length = 12;
	bundle->req.tce.image_size = htole32(sc->sc_fw_len);

	bundle->sge.sg_hdr = htole32(MPI_SGE_FL_TYPE_SIMPLE |
	    MPI_SGE_FL_SIZE_64 | MPI_SGE_FL_LAST | MPI_SGE_FL_EOB |
	    MPI_SGE_FL_EOL | (u_int32_t)sc->sc_fw_len);
	addr = MPI_DMA_DVA(sc->sc_fw);
	bundle->sge.sg_hi_addr = htole32((u_int32_t)(addr >> 32));
	bundle->sge.sg_lo_addr = htole32((u_int32_t)addr);

	if (mpi_poll(sc, ccb, 50000) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_header poll\n", DEVNAME(sc));
		goto err;
	}

	if (ccb->ccb_rcb == NULL)
		panic("%s: unable to do fw upload\n", DEVNAME(sc));
	upp = ccb->ccb_rcb->rcb_reply;

	if (letoh16(upp->ioc_status) != MPI_IOCSTATUS_SUCCESS)
		rv = 1;

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	mpi_put_ccb(sc, ccb);

	return (rv);

err:
	mpi_dmamem_free(sc, sc->sc_fw);
	return (1);
}

void
mpi_get_raid(struct mpi_softc *sc)
{
	struct mpi_cfg_hdr		hdr;
	struct mpi_cfg_ioc_pg2		*vol_page;
	struct mpi_cfg_raid_vol		*vol_list, *vol;
	size_t				pagelen;
	u_int32_t			capabilities;
	struct scsi_link		*link;
	int				i;

	DNPRINTF(MPI_D_RAID, "%s: mpi_get_raid\n", DEVNAME(sc));

	if (mpi_cfg_header(sc, MPI_CONFIG_REQ_PAGE_TYPE_IOC, 2, 0, &hdr) != 0) {
		DNPRINTF(MPI_D_RAID, "%s: mpi_get_raid unable to fetch header"
		    "for IOC page 2\n", DEVNAME(sc));
		return;
	}

	pagelen = hdr.page_length * 4; /* dwords to bytes */
	vol_page = malloc(pagelen, M_TEMP, M_WAITOK);
	if (vol_page == NULL) {
		DNPRINTF(MPI_D_RAID, "%s: mpi_get_raid unable to allocate "
		    "space for ioc config page 2\n", DEVNAME(sc));
		return;
	}
	vol_list = (struct mpi_cfg_raid_vol *)(vol_page + 1);

	if (mpi_cfg_page(sc, 0, &hdr, 1, vol_page, pagelen) != 0) {
		DNPRINTF(MPI_D_RAID, "%s: mpi_get_raid unable to fetch IOC "
		    "page 2\n", DEVNAME(sc));
		goto out;
	}

	capabilities = letoh32(vol_page->capabilities);

	DNPRINTF(MPI_D_RAID, "%s:  capabilities: 0x08%x\n", DEVNAME(sc),
	    letoh32(vol_page->capabilities));
	DNPRINTF(MPI_D_RAID, "%s:  active_vols: %d max_vols: %d "
	    "active_physdisks: %d max_physdisks: %d\n", DEVNAME(sc),
	    vol_page->active_vols, vol_page->max_vols,
	    vol_page->active_physdisks, vol_page->max_physdisks);

	/* don't walk list if there are no RAID capability */
	if (capabilities == 0xdeadbeef) {
		printf("%s: deadbeef in raid configuration\n", DEVNAME(sc));
		goto out;
	}

	if ((capabilities & MPI_CFG_IOC_2_CAPABILITIES_RAID) == 0 ||
	    (vol_page->active_vols == 0))
		goto out;

	sc->sc_flags |= MPI_F_RAID;

	for (i = 0; i < vol_page->active_vols; i++) {
		vol = &vol_list[i];

		DNPRINTF(MPI_D_RAID, "%s:   id: %d bus: %d ioc: %d pg: %d\n",
		    DEVNAME(sc), vol->vol_id, vol->vol_bus, vol->vol_ioc,
		    vol->vol_page);
		DNPRINTF(MPI_D_RAID, "%s:   type: 0x%02x flags: 0x%02x\n",
		    DEVNAME(sc), vol->vol_type, vol->flags);

		if (vol->vol_ioc != sc->sc_ioc_number || vol->vol_bus != 0)
			continue;

		link = sc->sc_scsibus->sc_link[vol->vol_id][0];
		if (link == NULL)
			continue;

		link->flags |= SDEV_VIRTUAL;
	}

out:
	free(vol_page, M_TEMP);
}

int
mpi_cfg_header(struct mpi_softc *sc, u_int8_t type, u_int8_t number,
    u_int32_t address, struct mpi_cfg_hdr *hdr)
{
	struct mpi_ccb				*ccb;
	struct mpi_msg_config_request		*cq;
	struct mpi_msg_config_reply		*cp;
	int					rv = 0;
	int					s;

	DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_header type: %#x number: %x "
	    "address: %d\n", DEVNAME(sc), type, number, address);

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_header ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	ccb->ccb_done = mpi_empty_done;
	cq = ccb->ccb_cmd;

	cq->function = MPI_FUNCTION_CONFIG;
	cq->msg_context = htole32(ccb->ccb_id);

	cq->action = MPI_CONFIG_REQ_ACTION_PAGE_HEADER;

	cq->config_header.page_number = number;
	cq->config_header.page_type = type;
	cq->page_address = htole32(address);
	cq->page_buffer.sg_hdr = htole32(MPI_SGE_FL_TYPE_SIMPLE |
	    MPI_SGE_FL_LAST | MPI_SGE_FL_EOB | MPI_SGE_FL_EOL);

	if (mpi_poll(sc, ccb, 50000) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_header poll\n", DEVNAME(sc));
		return (1);
	}

	if (ccb->ccb_rcb == NULL)
		panic("%s: unable to fetch config header\n", DEVNAME(sc));
	cp = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPI_D_MISC, "%s:  action: 0x%02x msg_length: %d function: "
	    "0x%02x\n", DEVNAME(sc), cp->action, cp->msg_length, cp->function);
	DNPRINTF(MPI_D_MISC, "%s:  ext_page_length: %d ext_page_type: 0x%02x "
	    "msg_flags: 0x%02x\n", DEVNAME(sc),
	    letoh16(cp->ext_page_length), cp->ext_page_type,
	    cp->msg_flags);
	DNPRINTF(MPI_D_MISC, "%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(cp->msg_context));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    letoh16(cp->ioc_status));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(cp->ioc_loginfo));
	DNPRINTF(MPI_D_MISC, "%s:  page_version: 0x%02x page_length: %d "
	    "page_number: 0x%02x page_type: 0x%02x\n", DEVNAME(sc),
	    cp->config_header.page_version,
	    cp->config_header.page_length,
	    cp->config_header.page_number,
	    cp->config_header.page_type);

	if (letoh16(cp->ioc_status) != MPI_IOCSTATUS_SUCCESS)
		rv = 1;
	else
		*hdr = cp->config_header;

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	mpi_put_ccb(sc, ccb);

	return (rv);
}

int
mpi_cfg_page(struct mpi_softc *sc, u_int32_t address, struct mpi_cfg_hdr *hdr,
    int read, void *page, size_t len)
{
	struct mpi_ccb				*ccb;
	struct mpi_msg_config_request		*cq;
	struct mpi_msg_config_reply		*cp;
	u_int64_t				dva;
	char					*kva;
	int					rv = 0;
	int					s;

	DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_page address: %d read: %d type: %x\n",
	    DEVNAME(sc), address, read, hdr->page_type);

	if (len > MPI_REQUEST_SIZE - sizeof(struct mpi_msg_config_request) ||
	    len < hdr->page_length * 4)
		return (1);

	s = splbio();
	ccb = mpi_get_ccb(sc);
	splx(s);
	if (ccb == NULL) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_page ccb_get\n", DEVNAME(sc));
		return (1);
	}

	ccb->ccb_done = mpi_empty_done;
	cq = ccb->ccb_cmd;

	cq->function = MPI_FUNCTION_CONFIG;
	cq->msg_context = htole32(ccb->ccb_id);

	cq->action = (read ? MPI_CONFIG_REQ_ACTION_PAGE_READ_CURRENT :
	    MPI_CONFIG_REQ_ACTION_PAGE_WRITE_CURRENT);

	cq->config_header = *hdr;
	cq->config_header.page_type &= MPI_CONFIG_REQ_PAGE_TYPE_MASK;
	cq->page_address = htole32(address);
	cq->page_buffer.sg_hdr = htole32(MPI_SGE_FL_TYPE_SIMPLE |
	    MPI_SGE_FL_LAST | MPI_SGE_FL_EOB | MPI_SGE_FL_EOL |
	    (hdr->page_length * 4) |
	    (read ? MPI_SGE_FL_DIR_IN : MPI_SGE_FL_DIR_OUT));

	/* bounce the page via the request space to avoid more bus_dma games */
	dva = ccb->ccb_cmd_dva + sizeof(struct mpi_msg_config_request);

	cq->page_buffer.sg_hi_addr = htole32((u_int32_t)(dva >> 32));
	cq->page_buffer.sg_lo_addr = htole32((u_int32_t)dva);

	kva = ccb->ccb_cmd;
	kva += sizeof(struct mpi_msg_config_request);
	if (!read)
		bcopy(page, kva, len);

	if (mpi_poll(sc, ccb, 50000) != 0) {
		DNPRINTF(MPI_D_MISC, "%s: mpi_cfg_page poll\n", DEVNAME(sc));
		return (1);
	}

	if (ccb->ccb_rcb == NULL) {
		mpi_put_ccb(sc, ccb);
		return (1);
	}
	cp = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPI_D_MISC, "%s:  action: 0x%02x msg_length: %d function: "
	    "0x%02x\n", DEVNAME(sc), cp->action, cp->msg_length, cp->function);
	DNPRINTF(MPI_D_MISC, "%s:  ext_page_length: %d ext_page_type: 0x%02x "
	    "msg_flags: 0x%02x\n", DEVNAME(sc),
	    letoh16(cp->ext_page_length), cp->ext_page_type,
	    cp->msg_flags);
	DNPRINTF(MPI_D_MISC, "%s:  msg_context: 0x%08x\n", DEVNAME(sc),
	    letoh32(cp->msg_context));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    letoh16(cp->ioc_status));
	DNPRINTF(MPI_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    letoh32(cp->ioc_loginfo));
	DNPRINTF(MPI_D_MISC, "%s:  page_version: 0x%02x page_length: %d "
	    "page_number: 0x%02x page_type: 0x%02x\n", DEVNAME(sc),
	    cp->config_header.page_version,
	    cp->config_header.page_length,
	    cp->config_header.page_number,
	    cp->config_header.page_type);

	if (letoh16(cp->ioc_status) != MPI_IOCSTATUS_SUCCESS)
		rv = 1;
	else if (read)
		bcopy(kva, page, len);

	mpi_push_reply(sc, ccb->ccb_rcb->rcb_reply_dva);
	mpi_put_ccb(sc, ccb);

	return (rv);
}

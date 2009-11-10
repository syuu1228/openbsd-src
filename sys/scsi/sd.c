/*	$OpenBSD: src/sys/scsi/sd.c,v 1.163 2009/11/10 10:18:59 dlg Exp $	*/
/*	$NetBSD: sd.c,v 1.111 1997/04/02 02:29:41 mycroft Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timeout.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/scsiio.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>
#include <scsi/sdvar.h>

#include <ufs/ffs/fs.h>			/* for BBSIZE and SBSIZE */

#include <sys/vnode.h>

int	sdmatch(struct device *, void *, void *);
void	sdattach(struct device *, struct device *, void *);
int	sdactivate(struct device *, int);
int	sddetach(struct device *, int);

void	sdminphys(struct buf *);
int	sdgetdisklabel(dev_t, struct sd_softc *, struct disklabel *, int);
void	sdstart(void *);
void	sd_shutdown(void *);
int	sd_interpret_sense(struct scsi_xfer *);
int	sd_get_parms(struct sd_softc *, struct disk_parms *, int);
void	sd_flush(struct sd_softc *, int);
void	sd_kill_buffers(struct sd_softc *);

void	viscpy(u_char *, u_char *, int);

int	sd_ioctl_inquiry(struct sd_softc *, struct dk_inquiry *);

struct buf *sd_buf_dequeue(struct sd_softc *);
void	sd_buf_requeue(struct sd_softc *, struct buf *);

void	sd_cmd_rw6(struct scsi_xfer *, int, daddr64_t, u_int);
void	sd_cmd_rw10(struct scsi_xfer *, int, daddr64_t, u_int);
void	sd_cmd_rw12(struct scsi_xfer *, int, daddr64_t, u_int);
void	sd_cmd_rw16(struct scsi_xfer *, int, daddr64_t, u_int);

void	sd_buf_done(struct scsi_xfer *);

struct cfattach sd_ca = {
	sizeof(struct sd_softc), sdmatch, sdattach,
	sddetach, sdactivate
};

struct cfdriver sd_cd = {
	NULL, "sd", DV_DISK
};

struct dkdriver sddkdriver = { sdstrategy };

struct scsi_device sd_switch = {
	sd_interpret_sense,	/* check out error handler first */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* have no done handler */
};

const struct scsi_inquiry_pattern sd_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_RDIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_RDIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
};

#define sdlock(softc)   disk_lock(&(softc)->sc_dk)
#define sdunlock(softc) disk_unlock(&(softc)->sc_dk)
#define sdlookup(unit) (struct sd_softc *)device_lookup(&sd_cd, (unit))

int
sdmatch(struct device *parent, void *match, void *aux)
{
	struct scsi_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    sd_patterns, sizeof(sd_patterns)/sizeof(sd_patterns[0]),
	    sizeof(sd_patterns[0]), &priority);

	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
sdattach(struct device *parent, struct device *self, void *aux)
{
	int error, result;
	struct sd_softc *sd = (struct sd_softc *)self;
	struct disk_parms *dp = &sd->params;
	struct scsi_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;
	int sd_autoconf = scsi_autoconf | SCSI_SILENT |
	    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdattach:\n"));

	mtx_init(&sd->sc_buf_mtx, IPL_BIO);

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_link = sc_link;
	sc_link->device = &sd_switch;
	sc_link->device_softc = sd;

	/*
	 * Initialize and attach the disk structure.
	 */
	sd->sc_dk.dk_driver = &sddkdriver;
	sd->sc_dk.dk_name = sd->sc_dev.dv_xname;
	disk_attach(&sd->sc_dk);

	if ((sc_link->flags & SDEV_ATAPI) && (sc_link->flags & SDEV_REMOVABLE))
		sc_link->quirks |= SDEV_NOSYNCCACHE;

	if (!(sc_link->inqdata.flags & SID_RelAdr))
		sc_link->quirks |= SDEV_ONLYBIG;

	/*
	 * Note if this device is ancient.  This is used in sdminphys().
	 */
	if (!(sc_link->flags & SDEV_ATAPI) &&
	    SCSISPC(sa->sa_inqbuf->version) == 0)
		sd->flags |= SDF_ANCIENT;

	/*
	 * Use the subdriver to request information regarding
	 * the drive. We cannot use interrupts yet, so the
	 * request must specify this.
	 */
	printf("\n");

	timeout_set(&sd->sc_timeout, sdstart, sd);

	/* Spin up non-UMASS devices ready or not. */
	if ((sd->sc_link->flags & SDEV_UMASS) == 0)
		scsi_start(sc_link, SSS_START, sd_autoconf);

	/*
	 * Some devices (e.g. Blackberry Pearl) won't admit they have
	 * media loaded unless its been locked in.
	 */
	if ((sc_link->flags & SDEV_REMOVABLE) != 0)
		scsi_prevent(sc_link, PR_PREVENT, sd_autoconf);

	/* Check that it is still responding and ok. */
	error = scsi_test_unit_ready(sd->sc_link, TEST_READY_RETRIES * 3,
	    sd_autoconf);

	if (error)
		result = SDGP_RESULT_OFFLINE;
	else
		result = sd_get_parms(sd, &sd->params, sd_autoconf);

	if ((sc_link->flags & SDEV_REMOVABLE) != 0)
		scsi_prevent(sc_link, PR_ALLOW, sd_autoconf);

	printf("%s: ", sd->sc_dev.dv_xname);
	switch (result) {
	case SDGP_RESULT_OK:
		printf("%lldMB, %lu bytes/sec, %lld sec total",
		    dp->disksize / (1048576 / dp->blksize), dp->blksize,
		    dp->disksize);
		break;

	case SDGP_RESULT_OFFLINE:
		printf("drive offline");
		break;

#ifdef DIAGNOSTIC
	default:
		panic("sdattach: unknown result (%#x) from get_parms", result);
		break;
#endif
	}
	printf("\n");

	/*
	 * Establish a shutdown hook so that we can ensure that
	 * our data has actually made it onto the platter at
	 * shutdown time.  Note that this relies on the fact
	 * that the shutdown hook code puts us at the head of
	 * the list (thus guaranteeing that our hook runs before
	 * our ancestors').
	 */
	if ((sd->sc_sdhook =
	    shutdownhook_establish(sd_shutdown, sd)) == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sd->sc_dev.dv_xname);
}

int
sdactivate(struct device *self, int act)
{
	struct sd_softc *sd = (struct sd_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		sd->flags |= SDF_DYING;
		sd_kill_buffers(sd);
		break;
	}

	return (rv);
}


int
sddetach(struct device *self, int flags)
{
	struct sd_softc *sd = (struct sd_softc *)self;
	int bmaj, cmaj, mn;

	sd_kill_buffers(sd);

	/* Locate the lowest minor number to be detached. */
	mn = DISKMINOR(self->dv_unit, 0);

	for (bmaj = 0; bmaj < nblkdev; bmaj++)
		if (bdevsw[bmaj].d_open == sdopen)
			vdevgone(bmaj, mn, mn + MAXPARTITIONS - 1, VBLK);
	for (cmaj = 0; cmaj < nchrdev; cmaj++)
		if (cdevsw[cmaj].d_open == sdopen)
			vdevgone(cmaj, mn, mn + MAXPARTITIONS - 1, VCHR);

	/* Get rid of the shutdown hook. */
	if (sd->sc_sdhook != NULL)
		shutdownhook_disestablish(sd->sc_sdhook);

	/* Detach disk. */
	disk_detach(&sd->sc_dk);

	return (0);
}

/*
 * Open the device. Make sure the partition info is as up-to-date as can be.
 */
int
sdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct scsi_link *sc_link;
	struct sd_softc *sd;
	int error = 0, part, rawopen, unit;

	unit = DISKUNIT(dev);
	part = DISKPART(dev);

	rawopen = (part == RAW_PART) && (fmt == S_IFCHR);

	sd = sdlookup(unit);
	if (sd == NULL)
		return (ENXIO);
	if (sd->flags & SDF_DYING) {
		device_unref(&sd->sc_dev);
		return (ENXIO);
	}

	sc_link = sd->sc_link;
	SC_DEBUG(sc_link, SDEV_DB1,
	    ("sdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    sd_cd.cd_ndevs, part));

	if ((error = sdlock(sd)) != 0) {
		device_unref(&sd->sc_dev);
		return (error);
	}

	if (sd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens of non-raw partition.
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			if (rawopen)
				goto out;
			error = EIO;
			goto bad;
		}
	} else {
		/* Spin up non-UMASS devices ready or not. */
		if ((sd->sc_link->flags & SDEV_UMASS) == 0)
			scsi_start(sc_link, SSS_START, (rawopen ? SCSI_SILENT :
			    0) | SCSI_IGNORE_ILLEGAL_REQUEST |
			    SCSI_IGNORE_MEDIA_CHANGE);

		/* Use sd_interpret_sense() for sense errors.
		 *
		 * But only after spinning the disk up! Just in case a broken
		 * device returns "Initialization command required." and causes
		 * a loop of scsi_start() calls.
		 */
		sc_link->flags |= SDEV_OPEN;

		/*
		 * Try to prevent the unloading of a removable device while
		 * it's open. But allow the open to proceed if the device can't
		 * be locked in.
		 */
		if ((sc_link->flags & SDEV_REMOVABLE) != 0) {
			scsi_prevent(sc_link, PR_PREVENT, SCSI_SILENT |
			    SCSI_IGNORE_ILLEGAL_REQUEST |
			    SCSI_IGNORE_MEDIA_CHANGE);
		}

		/* Check that it is still responding and ok. */
		error = scsi_test_unit_ready(sc_link,
		    TEST_READY_RETRIES, SCSI_SILENT |
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);

		if (error) {
			if (rawopen) {
				error = 0;
				goto out;
			} else
				goto bad;
		}

		/* Load the physical device parameters. */
		sc_link->flags |= SDEV_MEDIA_LOADED;
		if (sd_get_parms(sd, &sd->params, (rawopen ? SCSI_SILENT : 0))
		    == SDGP_RESULT_OFFLINE) {
			sc_link->flags &= ~SDEV_MEDIA_LOADED;
			error = ENXIO;
			goto bad;
		}
		SC_DEBUG(sc_link, SDEV_DB3, ("Params loaded\n"));

		/* Load the partition info if not already loaded. */
		if (sdgetdisklabel(dev, sd, sd->sc_dk.dk_label, 0) == EIO) {
			error = EIO;
			goto bad;
		}
		SC_DEBUG(sc_link, SDEV_DB3, ("Disklabel loaded\n"));
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= sd->sc_dk.dk_label->d_npartitions ||
	    sd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

out:	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;
	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));

	/* It's OK to fall through because dk_openmask is now non-zero. */
bad:
	if (sd->sc_dk.dk_openmask == 0) {
		if ((sd->sc_link->flags & SDEV_REMOVABLE) != 0)
			scsi_prevent(sc_link, PR_ALLOW, SCSI_SILENT |
			    SCSI_IGNORE_ILLEGAL_REQUEST |
			    SCSI_IGNORE_MEDIA_CHANGE);
		sc_link->flags &= ~(SDEV_OPEN | SDEV_MEDIA_LOADED);
	}

	sdunlock(sd);
	device_unref(&sd->sc_dev);
	return (error);
}

/*
 * Close the device. Only called if we are the last occurrence of an open
 * device.  Convenient now but usually a pain.
 */
int
sdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct sd_softc *sd;
	int part = DISKPART(dev);
	int error;

	sd = sdlookup(DISKUNIT(dev));
	if (sd == NULL)
		return (ENXIO);
	if (sd->flags & SDF_DYING) {
		device_unref(&sd->sc_dev);
		return (ENXIO);
	}

	if ((error = sdlock(sd)) != 0) {
		device_unref(&sd->sc_dev);
		return (error);
	}

	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	if (sd->sc_dk.dk_openmask == 0) {
		if ((sd->flags & SDF_DIRTY) != 0)
			sd_flush(sd, 0);

		if ((sd->sc_link->flags & SDEV_REMOVABLE) != 0)
			scsi_prevent(sd->sc_link, PR_ALLOW,
			    SCSI_IGNORE_ILLEGAL_REQUEST |
			    SCSI_IGNORE_NOT_READY | SCSI_SILENT);
		sd->sc_link->flags &= ~(SDEV_OPEN | SDEV_MEDIA_LOADED);

		if (sd->sc_link->flags & SDEV_EJECTING) {
			scsi_start(sd->sc_link, SSS_STOP|SSS_LOEJ, 0);
			sd->sc_link->flags &= ~SDEV_EJECTING;
		}

		timeout_del(&sd->sc_timeout);
	}

	sdunlock(sd);
	device_unref(&sd->sc_dev);
	return 0;
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
sdstrategy(struct buf *bp)
{
	struct sd_softc *sd;
	int s;

	sd = sdlookup(DISKUNIT(bp->b_dev));
	if (sd == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}
	if (sd->flags & SDF_DYING) {
		bp->b_error = ENXIO;
		goto bad;
	}

	SC_DEBUG(sd->sc_link, SDEV_DB2, ("sdstrategy: %ld bytes @ blk %d\n",
	    bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 */
	if ((sd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		if (sd->sc_link->flags & SDEV_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediately
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * The transfer must be a whole number of sectors.
	 */
	if ((bp->b_bcount % sd->sc_dk.dk_label->d_secsize) != 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (bounds_check_with_label(bp, sd->sc_dk.dk_label,
	    (sd->flags & (SDF_WLABEL|SDF_LABELLING)) != 0) <= 0)
		goto done;

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	mtx_enter(&sd->sc_buf_mtx);
	disksort(&sd->sc_buf_queue, bp);
	mtx_leave(&sd->sc_buf_mtx);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	sdstart(sd);

	device_unref(&sd->sc_dev);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	s = splbio();
	biodone(bp);
	splx(s);
	if (sd != NULL)
		device_unref(&sd->sc_dev);
}

struct buf *
sd_buf_dequeue(struct sd_softc *sc)
{
	struct buf *bp;

	mtx_enter(&sc->sc_buf_mtx);
	bp = sc->sc_buf_queue.b_actf;
	if (bp != NULL)
		sc->sc_buf_queue.b_actf = bp->b_actf;
	mtx_leave(&sc->sc_buf_mtx);

	return (bp);
}

void
sd_buf_requeue(struct sd_softc *sc, struct buf *bp)
{
	mtx_enter(&sc->sc_buf_mtx);
	bp->b_actf = sc->sc_buf_queue.b_actf;
	sc->sc_buf_queue.b_actf = bp;
	mtx_leave(&sc->sc_buf_mtx);
}

void
sd_cmd_rw6(struct scsi_xfer *xs, int read, daddr64_t blkno, u_int nblks)
{
	struct scsi_rw *cmd = (struct scsi_rw *)xs->cmd;

	cmd->opcode = read ? READ_COMMAND : WRITE_COMMAND;
	_lto3b(blkno, cmd->addr);
	cmd->length = nblks;

	xs->cmdlen = sizeof(*cmd);
}

void
sd_cmd_rw10(struct scsi_xfer *xs, int read, daddr64_t blkno, u_int nblks)
{
	struct scsi_rw_big *cmd = (struct scsi_rw_big *)xs->cmd;

	cmd->opcode = read ? READ_BIG : WRITE_BIG;
	_lto4b(blkno, cmd->addr);
	_lto2b(nblks, cmd->length);

	xs->cmdlen = sizeof(*cmd);
}

void
sd_cmd_rw12(struct scsi_xfer *xs, int read, daddr64_t blkno, u_int nblks)
{
	struct scsi_rw_12 *cmd = (struct scsi_rw_12 *)xs->cmd;

	cmd->opcode = read ? READ_12 : WRITE_12;
	_lto4b(blkno, cmd->addr);
	_lto4b(nblks, cmd->length);

	xs->cmdlen = sizeof(*cmd);
}

void
sd_cmd_rw16(struct scsi_xfer *xs, int read, daddr64_t blkno, u_int nblks)
{
	struct scsi_rw_16 *cmd = (struct scsi_rw_16 *)xs->cmd;

	cmd->opcode = read ? READ_16 : WRITE_16;
	_lto8b(blkno, cmd->addr);
	_lto4b(nblks, cmd->length);

	xs->cmdlen = sizeof(*cmd);
}

/*
 * sdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (sdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 */
void
sdstart(void *v)
{
	struct sd_softc *sc = (struct sd_softc *)v;
	struct scsi_link *link = sc->sc_link;
	struct scsi_xfer *xs;
	struct buf *bp;
	daddr64_t blkno;
	int nblks;
	int read;
	struct partition *p;

	if (sc->flags & SDF_DYING)
		return;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdstart\n"));

	CLR(sc->flags, SDF_WAITING);
	while (!ISSET(sc->flags, SDF_WAITING) &&
	    (bp = sd_buf_dequeue(sc)) != NULL) {
		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if ((link->flags & SDEV_MEDIA_LOADED) == 0) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		xs = scsi_xs_get(link, SCSI_NOSLEEP);
		if (xs == NULL) {
			sd_buf_requeue(sc, bp);
			return;
		}

		blkno =
		    bp->b_blkno / (sc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
		p = &sc->sc_dk.dk_label->d_partitions[DISKPART(bp->b_dev)];
		blkno += DL_GETPOFFSET(p);
		nblks = howmany(bp->b_bcount, sc->sc_dk.dk_label->d_secsize);
		read = bp->b_flags & B_READ;

		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (!(link->flags & SDEV_ATAPI) &&
		    !(link->quirks & SDEV_ONLYBIG) &&
		    ((blkno & 0x1fffff) == blkno) &&
		    ((nblks & 0xff) == nblks))
			sd_cmd_rw6(xs, read, blkno, nblks);
		else if (((blkno & 0xffffffff) == blkno) &&
		    ((nblks & 0xffff) == nblks))
			sd_cmd_rw10(xs, read, blkno, nblks);
		else if (((blkno & 0xffffffff) == blkno) &&
		    ((nblks & 0xffffffff) == nblks))
			sd_cmd_rw12(xs, read, blkno, nblks);
		else
			sd_cmd_rw16(xs, read, blkno, nblks);

		xs->flags |= (read ? SCSI_DATA_IN : SCSI_DATA_OUT);
		xs->timeout = 60000;
		xs->data = bp->b_data;
		xs->datalen = bp->b_bcount;

		xs->done = sd_buf_done;
		xs->cookie = bp;

		/* Instrumentation. */
		disk_busy(&sc->sc_dk);
		scsi_xs_exec(xs);
	}
}

void
sd_buf_done(struct scsi_xfer *xs)
{
	struct sd_softc *sc = xs->sc_link->device_softc;
	struct buf *bp = xs->cookie;

	splassert(IPL_BIO);

	disk_unbusy(&sc->sc_dk, bp->b_bcount - xs->resid,
	    bp->b_flags & B_READ);

	switch (xs->error) {
	case XS_NOERROR:
		bp->b_error = 0;
		bp->b_resid = xs->resid;
		break;

	case XS_NO_CCB:
		/* The hardware is busy, requeue the buf and try it later. */
		sd_buf_requeue(sc, bp);
		scsi_xs_put(xs);
		SET(sc->flags, SDF_WAITING); /* break out of sdstart loop */
		timeout_add(&sc->sc_timeout, 1);
		return;

	default:
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		break;
	}

	biodone(bp);
	scsi_xs_put(xs);
	sdstart(sc); /* restart io */
}

void
sdminphys(struct buf *bp)
{
	struct sd_softc *sd;
	long max;

	sd = sdlookup(DISKUNIT(bp->b_dev));
	if (sd == NULL)
		return;  /* XXX - right way to fail this? */

	/*
	 * If the device is ancient, we want to make sure that
	 * the transfer fits into a 6-byte cdb.
	 *
	 * XXX Note that the SCSI-I spec says that 256-block transfers
	 * are allowed in a 6-byte read/write, and are specified
	 * by setting the "length" to 0.  However, we're conservative
	 * here, allowing only 255-block transfers in case an
	 * ancient device gets confused by length == 0.  A length of 0
	 * in a 10-byte read/write actually means 0 blocks.
	 */
	if (sd->flags & SDF_ANCIENT) {
		max = sd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > max)
			bp->b_bcount = max;
	}

	(*sd->sc_link->adapter->scsi_minphys)(bp, sd->sc_link);

	device_unref(&sd->sc_dev);
}

int
sdread(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(sdstrategy, NULL, dev, B_READ, sdminphys, uio));
}

int
sdwrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(sdstrategy, NULL, dev, B_WRITE, sdminphys, uio));
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
 */
int
sdioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	struct sd_softc *sd;
	struct disklabel *lp;
	int error = 0;
	int part = DISKPART(dev);

	sd = sdlookup(DISKUNIT(dev));
	if (sd == NULL)
		return (ENXIO);
	if (sd->flags & SDF_DYING) {
		device_unref(&sd->sc_dev);
		return (ENXIO);
	}

	SC_DEBUG(sd->sc_link, SDEV_DB2, ("sdioctl 0x%lx\n", cmd));

	/*
	 * If the device is not valid.. abandon ship
	 */
	if ((sd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		switch (cmd) {
		case DIOCWLABEL:
		case DIOCLOCK:
		case DIOCEJECT:
		case SCIOCIDENTIFY:
		case SCIOCCOMMAND:
		case SCIOCDEBUG:
			if (part == RAW_PART)
				break;
		/* FALLTHROUGH */
		default:
			if ((sd->sc_link->flags & SDEV_OPEN) == 0) {
				error = ENODEV;
				goto exit;
			} else {
				error = EIO;
				goto exit;
			}
		}
	}

	switch (cmd) {
	case DIOCRLDINFO:
		lp = malloc(sizeof(*lp), M_TEMP, M_WAITOK);
		sdgetdisklabel(dev, sd, lp, 0);
		bcopy(lp, sd->sc_dk.dk_label, sizeof(*lp));
		free(lp, M_TEMP);
		goto exit;
	case DIOCGPDINFO:
		sdgetdisklabel(dev, sd, (struct disklabel *)addr, 1);
		goto exit;

	case DIOCGDINFO:
		*(struct disklabel *)addr = *(sd->sc_dk.dk_label);
		goto exit;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = sd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sd->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		goto exit;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			goto exit;
		}

		if ((error = sdlock(sd)) != 0)
			goto exit;
		sd->flags |= SDF_LABELLING;

		error = setdisklabel(sd->sc_dk.dk_label,
		    (struct disklabel *)addr, /*sd->sc_dk.dk_openmask : */0);
		if (error == 0) {
			if (cmd == DIOCWDINFO)
				error = writedisklabel(DISKLABELDEV(dev),
				    sdstrategy, sd->sc_dk.dk_label);
		}

		sd->flags &= ~SDF_LABELLING;
		sdunlock(sd);
		goto exit;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			goto exit;
		}
		if (*(int *)addr)
			sd->flags |= SDF_WLABEL;
		else
			sd->flags &= ~SDF_WLABEL;
		goto exit;

	case DIOCLOCK:
		error = scsi_prevent(sd->sc_link,
		    (*(int *)addr) ? PR_PREVENT : PR_ALLOW, 0);
		goto exit;

	case MTIOCTOP:
		if (((struct mtop *)addr)->mt_op != MTOFFL) {
			error = EIO;
			goto exit;
		}
		/* FALLTHROUGH */
	case DIOCEJECT:
		if ((sd->sc_link->flags & SDEV_REMOVABLE) == 0) {
			error = ENOTTY;
			goto exit;
		}
		sd->sc_link->flags |= SDEV_EJECTING;
		goto exit;

	case DIOCINQ:
		error = scsi_do_ioctl(sd->sc_link, dev, cmd, addr, flag, p);
		if (error == ENOTTY)
			error = sd_ioctl_inquiry(sd,
			    (struct dk_inquiry *)addr);
		goto exit;

	default:
		if (part != RAW_PART) {
			error = ENOTTY;
			goto exit;
		}
		error = scsi_do_ioctl(sd->sc_link, dev, cmd, addr, flag, p);
	}

 exit:
	device_unref(&sd->sc_dev);
	return (error);
}

int
sd_ioctl_inquiry(struct sd_softc *sd, struct dk_inquiry *di)
{
	struct scsi_vpd_serial vpd;

	bzero(di, sizeof(struct dk_inquiry));
	scsi_strvis(di->vendor, sd->sc_link->inqdata.vendor,
	    sizeof(sd->sc_link->inqdata.vendor));
	scsi_strvis(di->product, sd->sc_link->inqdata.product,
	    sizeof(sd->sc_link->inqdata.product));
	scsi_strvis(di->revision, sd->sc_link->inqdata.revision,
	    sizeof(sd->sc_link->inqdata.revision));

	/* the serial vpd page is optional */
	if (scsi_inquire_vpd(sd->sc_link, &vpd, sizeof(vpd),
	    SI_PG_SERIAL, 0) == 0)
		scsi_strvis(di->serial, vpd.serial, sizeof(vpd.serial));
	else
		strlcpy(di->serial, "(unknown)", sizeof(vpd.serial));

	return (0);
}

/*
 * Load the label information on the named device
 */
int
sdgetdisklabel(dev_t dev, struct sd_softc *sd, struct disklabel *lp,
    int spoofonly)
{
	size_t len;
	char packname[sizeof(lp->d_packname) + 1];
	char product[17], vendor[9];

	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize = sd->params.blksize;
	lp->d_ntracks = sd->params.heads;
	lp->d_nsectors = sd->params.sectors;
	lp->d_ncylinders = sd->params.cyls;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	if (lp->d_secpercyl == 0) {
		lp->d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it */
	}

	lp->d_type = DTYPE_SCSI;
	if ((sd->sc_link->inqdata.device & SID_TYPE) == T_OPTICAL)
		strncpy(lp->d_typename, "SCSI optical",
		    sizeof(lp->d_typename));
	else
		strncpy(lp->d_typename, "SCSI disk",
		    sizeof(lp->d_typename));

	/*
	 * Try to fit '<vendor> <product>' into d_packname. If that doesn't fit
	 * then leave out '<vendor> ' and use only as much of '<product>' as
	 * does fit.
	 */
	viscpy(vendor, sd->sc_link->inqdata.vendor, 8);
	viscpy(product, sd->sc_link->inqdata.product, 16);
	len = snprintf(packname, sizeof(packname), "%s %s", vendor, product);
	if (len > sizeof(lp->d_packname)) {
		strlcpy(packname, product, sizeof(packname));
		len = strlen(packname);
	}
	/*
	 * It is safe to use len as the count of characters to copy because
	 * packname is sizeof(lp->d_packname)+1, the string in packname is
	 * always null terminated and len does not count the terminating null.
	 * d_packname is not a null terminated string.
	 */
	bcopy(packname, lp->d_packname, len);

	DL_SETDSIZE(lp, sd->params.disksize);
	lp->d_rpm = sd->params.rot_rate;
	lp->d_interleave = 1;
	lp->d_version = 1;
	lp->d_flags = 0;

	/* XXX - these values for BBSIZE and SBSIZE assume ffs */
	lp->d_bbsize = BBSIZE;
	lp->d_sbsize = SBSIZE;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);

	/*
	 * Call the generic disklabel extraction routine
	 */
	return readdisklabel(DISKLABELDEV(dev), sdstrategy, lp, spoofonly);
}


void
sd_shutdown(void *arg)
{
	struct sd_softc *sd = (struct sd_softc *)arg;

	/*
	 * If the disk cache needs to be flushed, and the disk supports
	 * it, flush it.  We're cold at this point, so we poll for
	 * completion.
	 */
	if ((sd->flags & SDF_DIRTY) != 0)
		sd_flush(sd, SCSI_AUTOCONF);

	timeout_del(&sd->sc_timeout);
}

/*
 * Check Errors
 */
int
sd_interpret_sense(struct scsi_xfer *xs)
{
	struct scsi_sense_data *sense = &xs->sense;
	struct scsi_link *sc_link = xs->sc_link;
	struct sd_softc *sd = sc_link->device_softc;
	u_int8_t serr = sense->error_code & SSD_ERRCODE;
	int retval;

	/*
	 * Let the generic code handle everything except a few categories of
	 * LUN not ready errors on open devices.
	 */
	if (((sc_link->flags & SDEV_OPEN) == 0) ||
	    (serr != SSD_ERRCODE_CURRENT && serr != SSD_ERRCODE_DEFERRED) ||
	    ((sense->flags & SSD_KEY) != SKEY_NOT_READY) ||
	    (sense->extra_len < 6))
		return (EJUSTRETURN);

	switch (ASC_ASCQ(sense)) {
	case SENSE_NOT_READY_BECOMING_READY:
		SC_DEBUG(sc_link, SDEV_DB1, ("becoming ready.\n"));
		retval = scsi_delay(xs, 5);
		break;

	case SENSE_NOT_READY_INIT_REQUIRED:
		SC_DEBUG(sc_link, SDEV_DB1, ("spinning up\n"));
		retval = scsi_start(sd->sc_link, SSS_START,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_URGENT | SCSI_NOSLEEP);
		if (retval == 0)
			retval = ERESTART;
		else
			SC_DEBUG(sc_link, SDEV_DB1, ("spin up failed (%#x)\n",
			    retval));
		break;

	default:
		retval = EJUSTRETURN;
		break;
	}

	return (retval);
}

daddr64_t
sdsize(dev_t dev)
{
	struct sd_softc *sd;
	int part, omask;
	int64_t size;

	sd = sdlookup(DISKUNIT(dev));
	if (sd == NULL)
		return -1;
	if (sd->flags & SDF_DYING) {
		size = -1;
		goto exit;
	}

	part = DISKPART(dev);
	omask = sd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && sdopen(dev, 0, S_IFBLK, NULL) != 0) {
		size = -1;
		goto exit;
	}
	if ((sd->sc_link->flags & SDEV_MEDIA_LOADED) == 0)
		size = -1;
	else if (sd->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = DL_GETPSIZE(&sd->sc_dk.dk_label->d_partitions[part]) *
			(sd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && sdclose(dev, 0, S_IFBLK, NULL) != 0)
		size = -1;

 exit:
	device_unref(&sd->sc_dev);
	return size;
}

/* #define SD_DUMP_NOT_TRUSTED if you just want to watch */
static int sddoingadump;

/*
 * dump all of physical memory into the partition specified, starting
 * at offset 'dumplo' into the partition.
 */
int
sddump(dev_t dev, daddr64_t blkno, caddr_t va, size_t size)
{
	struct sd_softc *sc;	/* disk unit to do the I/O */
	struct disklabel *lp;	/* disk's disklabel */
	int	unit, part;
	int	sectorsize;	/* size of a disk sector */
	daddr64_t	nsects;		/* number of sectors in partition */
	daddr64_t	sectoff;	/* sector offset of partition */
	int	totwrt;		/* total number of sectors left to write */
	int	nwrt;		/* current number of sectors to write */
	struct scsi_xfer *xs;	/* ... convenience */

	/* Check if recursive dump; if so, punt. */
	if (sddoingadump)
		return EFAULT;

	/* Mark as active early. */
	sddoingadump = 1;

	unit = DISKUNIT(dev);	/* Decompose unit & partition. */
	part = DISKPART(dev);

	/* Check for acceptable drive number. */
	if (unit >= sd_cd.cd_ndevs || (sc = sd_cd.cd_devs[unit]) == NULL)
		return ENXIO;

	/*
	 * XXX Can't do this check, since the media might have been
	 * XXX marked `invalid' by successful unmounting of all
	 * XXX filesystems.
	 */
#if 0
	/* Make sure it was initialized. */
	if ((sc->sc_link->flags & SDEV_MEDIA_LOADED) != SDEV_MEDIA_LOADED)
		return ENXIO;
#endif

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = sc->sc_dk.dk_label;
	sectorsize = lp->d_secsize;
	if ((size % sectorsize) != 0)
		return EFAULT;
	totwrt = size / sectorsize;
	blkno = dbtob(blkno) / sectorsize;	/* blkno in DEV_BSIZE units */

	nsects = DL_GETPSIZE(&lp->d_partitions[part]);
	sectoff = DL_GETPOFFSET(&lp->d_partitions[part]);

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + totwrt) > nsects))
		return EINVAL;

	/* Offset block number to start of partition. */
	blkno += sectoff;

	while (totwrt > 0) {
		nwrt = totwrt;		/* XXX */

#ifndef	SD_DUMP_NOT_TRUSTED
		xs = scsi_xs_get(sc->sc_link, SCSI_NOSLEEP);
		if (xs == NULL)
			return (ENOMEM);

		xs->timeout = 10000;
		xs->flags = SCSI_POLL | SCSI_NOSLEEP | SCSI_DATA_OUT;
		xs->data = va;
		xs->datalen = nwrt * sectorsize;

		sd_cmd_rw10(xs, 0, blkno, nwrt); /* XXX */

		scsi_xs_exec(xs);
		if (xs->error != XS_NOERROR)
			return (ENXIO);
		scsi_xs_put(xs);
#else	/* SD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("sd%d: dump addr 0x%x, blk %d\n", unit, va, blkno);
		delay(500 * 1000);	/* half a second */
#endif	/* SD_DUMP_NOT_TRUSTED */

		/* update block count */
		totwrt -= nwrt;
		blkno += nwrt;
		va += sectorsize * nwrt;
	}

	sddoingadump = 0;

	return (0);
}

/*
 * Copy up to len chars from src to dst, ignoring non-printables.
 * Must be room for len+1 chars in dst so we can write the NUL.
 * Does not assume src is NUL-terminated.
 */
void
viscpy(u_char *dst, u_char *src, int len)
{
	while (len > 0 && *src != '\0') {
		if (*src < 0x20 || *src >= 0x80) {
			src++;
			continue;
		}
		*dst++ = *src++;
		len--;
	}
	*dst = '\0';
}

/*
 * Fill out the disk parameter structure. Return SDGP_RESULT_OK if the
 * structure is correctly filled in, SDGP_RESULT_OFFLINE otherwise. The caller
 * is responsible for clearing the SDEV_MEDIA_LOADED flag if the structure
 * cannot be completed.
 */
int
sd_get_parms(struct sd_softc *sd, struct disk_parms *dp, int flags)
{
	union scsi_mode_sense_buf *buf = NULL;
	struct page_rigid_geometry *rigid;
	struct page_flex_geometry *flex;
	struct page_reduced_geometry *reduced;
	u_int32_t heads = 0, sectors = 0, cyls = 0, blksize = 0, ssblksize;
	u_int16_t rpm = 0;

	dp->disksize = scsi_size(sd->sc_link, flags, &ssblksize);

	/*
	 * Many UMASS devices choke when asked about their geometry. Most
	 * don't have a meaningful geometry anyway, so just fake it if
	 * scsi_size() worked.
	 */
	if ((sd->sc_link->flags & SDEV_UMASS) && (dp->disksize > 0))
		goto validate;	 /* N.B. buf will be NULL at validate. */

	buf = malloc(sizeof(*buf), M_TEMP, M_NOWAIT);
	if (buf == NULL)
		goto validate;

	switch (sd->sc_link->inqdata.device & SID_TYPE) {
	case T_OPTICAL:
		/* No more information needed or available. */
		break;

	case T_RDIRECT:
		/* T_RDIRECT supports only PAGE_REDUCED_GEOMETRY (6). */
		scsi_do_mode_sense(sd->sc_link, PAGE_REDUCED_GEOMETRY, buf,
		    (void **)&reduced, NULL, NULL, &blksize, sizeof(*reduced),
		    flags | SCSI_SILENT, NULL);
		if (DISK_PGCODE(reduced, PAGE_REDUCED_GEOMETRY)) {
			if (dp->disksize == 0)
				dp->disksize = _5btol(reduced->sectors);
			if (blksize == 0)
				blksize = _2btol(reduced->bytes_s);
		}
		break;

	default:
		/*
		 * NOTE: Some devices leave off the last four bytes of 
		 * PAGE_RIGID_GEOMETRY and PAGE_FLEX_GEOMETRY mode sense pages.
		 * The only information in those four bytes is RPM information
		 * so accept the page. The extra bytes will be zero and RPM will
		 * end up with the default value of 3600.
		 */
		rigid = NULL;
		if (((sd->sc_link->flags & SDEV_ATAPI) == 0) ||
		    ((sd->sc_link->flags & SDEV_REMOVABLE) == 0))
			scsi_do_mode_sense(sd->sc_link, PAGE_RIGID_GEOMETRY,
			    buf, (void **)&rigid, NULL, NULL, &blksize,
			    sizeof(*rigid) - 4, flags | SCSI_SILENT, NULL);
		if (DISK_PGCODE(rigid, PAGE_RIGID_GEOMETRY)) {
			heads = rigid->nheads;
			cyls = _3btol(rigid->ncyl);
			rpm = _2btol(rigid->rpm);
			if (heads * cyls > 0)
				sectors = dp->disksize / (heads * cyls);
		} else {
			scsi_do_mode_sense(sd->sc_link, PAGE_FLEX_GEOMETRY,
			    buf, (void **)&flex, NULL, NULL, &blksize,
			    sizeof(*flex) - 4, flags | SCSI_SILENT, NULL);
			if (DISK_PGCODE(flex, PAGE_FLEX_GEOMETRY)) {
				sectors = flex->ph_sec_tr;
				heads = flex->nheads;
				cyls = _2btol(flex->ncyl);
				rpm = _2btol(flex->rpm);
				if (blksize == 0)
					blksize = _2btol(flex->bytes_s);
				if (dp->disksize == 0)
					dp->disksize = heads * cyls * sectors;
			}
		}
		break;
	}

validate:
	if (buf)
		free(buf, M_TEMP);

	if (dp->disksize == 0)
		return (SDGP_RESULT_OFFLINE);

	if (ssblksize > 0)
		dp->blksize = ssblksize;
	else
		dp->blksize = (blksize == 0) ? 512 : blksize;

	/*
	 * Restrict blksize values to powers of two between 512 and 64k.
	 */
	switch (dp->blksize) {
	case 0x200:	/* == 512, == DEV_BSIZE on all architectures. */
	case 0x400:
	case 0x800:
	case 0x1000:
	case 0x2000:
	case 0x4000:
	case 0x8000:
	case 0x10000:
		break;
	default:
		SC_DEBUG(sd->sc_link, SDEV_DB1,
		    ("sd_get_parms: bad blksize: %#x\n", dp->blksize));
		return (SDGP_RESULT_OFFLINE);
	}

	/*
	 * XXX THINK ABOUT THIS!!  Using values such that sectors * heads *
	 * cyls is <= disk_size can lead to wasted space. We need a more
	 * careful calculation/validation to make everything work out
	 * optimally.
	 */
	if (dp->disksize > 0xffffffff && (dp->heads * dp->sectors) < 0xffff) {
		dp->heads = 511;
		dp->sectors = 255;
		cyls = 0;
	} else {
		/*
		 * Use standard geometry values for anything we still don't
		 * know.
		 */
		dp->heads = (heads == 0) ? 255 : heads;
		dp->sectors = (sectors == 0) ? 63 : sectors;
		dp->rot_rate = (rpm == 0) ? 3600 : rpm;
	}

	dp->cyls = (cyls == 0) ? dp->disksize / (dp->heads * dp->sectors) :
	    cyls;

	if (dp->cyls == 0) {
		dp->heads = dp->cyls = 1;
		dp->sectors = dp->disksize;
	}

	return (SDGP_RESULT_OK);
}
void
sd_flush_done(struct scsi_xfer *xs);

void
sd_flush(struct sd_softc *sc, int flags)
{
	struct scsi_link *link = sc->sc_link;
	struct scsi_xfer *xs;
	struct scsi_synchronize_cache *cmd;

	if (link->quirks & SDEV_NOSYNCCACHE)
		return;

	/*
	 * Issue a SYNCHRONIZE CACHE. Address 0, length 0 means "all remaining
	 * blocks starting at address 0". Ignore ILLEGAL REQUEST in the event
	 * that the command is not supported by the device.
	 */

	xs = scsi_xs_get(link, flags);
	if (xs == NULL) {
		SC_DEBUG(sc_link, SDEV_DB1, ("cache sync failed to get xs\n"));
		return;
	}

	cmd = (struct scsi_synchronize_cache *)xs->cmd;
	cmd->opcode = SYNCHRONIZE_CACHE;

	xs->timeout = 100000;

	xs->done = sd_flush_done;

	do {
		scsi_xs_exec(xs);
		if (!ISSET(xs->flags, SCSI_POLL)) {
			while (!ISSET(xs->flags, ITSDONE))
				tsleep(xs, PRIBIO, "sdflush", 0);
		}
	} while (xs->status == XS_NO_CCB);

	if (xs->error != XS_NOERROR)
		SC_DEBUG(sc_link, SDEV_DB1, ("cache sync failed\n"));
	else
		sc->flags &= ~SDF_DIRTY;

	scsi_xs_put(xs);
}

void
sd_flush_done(struct scsi_xfer *xs)
{
	if (!ISSET(xs->flags, SCSI_POLL))
		wakeup_one(xs);
}

/*
 * Remove unprocessed buffers from queue.
 */
void
sd_kill_buffers(struct sd_softc *sd)
{
	struct buf *bp;

	while ((bp = sd_buf_dequeue(sd)) != NULL) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
	}
}

/*	$OpenBSD: src/sys/arch/hp300/dev/Attic/st.c,v 1.10 1998/08/04 23:28:06 millert Exp $	*/
/*	$NetBSD: st.c,v 1.22 1997/04/02 22:37:38 scottr Exp $	*/

/*
 * Copyright (c) 1996, 1997 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1990 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: st.c 1.11 92/01/21$
 *
 *      @(#)st.c	8.3 (Berkeley) 1/12/94
 */

/*
 * SCSI CCS (Command Command Set) tape driver.
 *
 * Specific to Exabyte:
 * mt status: residual="Mbytes to LEOM"
 * minor bit 4 [b1bbbb] (aka /dev/rst16) selects short filemarks
 * minor bit 5 [1bbbbb] (aka /dev/rst32) selects fix block mode, 1k blocks.
 *
 * Archive drive:
 * can read both QIC-24 and QIC-II. But only writes
 * QIC-24.
 * 
 * Supports Archive Viper QIC-150 tape drive, but scsi.c reports selection
 * errors.
 *
 * Supports Archive Python DAT drive, but will sometimes hang machine.
 *
 * Supports HP 35450A DAT drive, but will sometimes hang machine.
 * Partitioning of tape not supported.
 * Vendor unique support has not been added.
 *
 *
 * Supports Archive VIPER (QIC-150).
 * Mostly Supports Archive PYTHON (DAT). 
 *     Hangs if write after spin down. 
 *     Need scsi.c that does connect/disconnect.
 */

/*
 * support for the block device not implemented 
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mtio.h>
#include <sys/kernel.h>
#include <sys/tprintf.h>
#include <sys/device.h>

#include <hp300/dev/scsireg.h>
#include <hp300/dev/scsivar.h>
#include <hp300/dev/stvar.h>

static struct scsi_fmt_cdb st_read_cmd = { 6, { CMD_READ } };
static struct scsi_fmt_cdb st_write_cmd = { 6, { CMD_WRITE } };

#define UNIT(x)		(minor(x) & 3)
#define stpunit(x)	((x) & 7)

#define STDEV_NOREWIND	0x04
#define STDEV_HIDENSITY	0x08
#define STDEV_EXSFMK	0x10
#define STDEV_FIXEDBLK	0x20

#ifdef DEBUG
int st_debug = 0x0000;
#define ST_OPEN		0x0001
#define ST_GO		0x0002
#define ST_FMKS		0x0004
#define ST_OPENSTAT	0x0008
#define ST_BRESID	0x0010
#define ST_ODDIO	0x0020
#endif

/*
 * Patchable variable.  If an even length read is requested a dma transfer is
 * used.  Only after the read will we find out if the read had an odd number
 * of bytes.  The HP98658 hardware cannot do odd length transfers, the last
 * byte of the data will always be 0x00.  Normally, the driver will complain
 * about such transfers and return EIO.  However, if st_dmaoddretry is non-
 * zero, the driver will try and issue a BSR and then re-read the data using
 * 'programmed transfer mode'.  In most cases this works, however for unknown
 * reasons it will hang the machine in certain cases.
 *
 * Note:
 * Odd length read requests are always done using programmed transfer mode.
 */
int st_dmaoddretry = 0;

/*
 * Exabyte only:
 * From adb can have access to fixed vs. variable length modes.
 * Use 0x400 for 1k (best capacity) fixed length records.
 * In st_open, if minor bit 4 set then 1k records are used.
 * If st_exblken is set to anything other then 0 we are in fixed length mode.
 * Minor bit 5 requests 1K fixed-length, overriding any setting of st_exblklen.
 */
int st_exblklen = 0;

/* exabyte here for adb access, set at open time */
#define EX_CT 	0x80		/* international cart - more space W/P6  */
#define EX_ND	0x20		/* no disconnect  */
#define EX_NBE	0x08		/* no busy enable  */
#define EX_EBD	0x04		/* even byte disconnect  */
#define EX_PE	0x02		/* parity enable  */
#define EX_NAL	0x01		/* no auto load  */
int st_exvup = (EX_CT|EX_ND|EX_NBE); /* vendor unique parameters */

/*
 * motion and reconnect thresholds guidelines:
 * write operation; lower motion threshold for faster transfer
 *                  raise reconnect threshold for slower transfer
 * read operation; lower motion threshold for slower transfer
 *                 raise reconnect threshold for faster transfer
 */
int st_exmotthr = 0x80;		/* motion threshold, 0x80 default */
int st_exreconthr = 0xa0;	/* reconnect threshold, 0xa0 default */
int st_exgapthr = 7;		/* gap threshold, 7 default */
#ifdef TTI
int st_extti = 0x01;		/* bitmask of unit numbers, do extra */
				/* sensing so TTi display gets updated */
#endif

/* bdev_decl(st); */
/* cdev_decl(st); */
/* XXX we should use macros to do these... */
int	stopen __P((dev_t, int, int, struct proc *));
int	stclose __P((dev_t, int, int, struct proc *));

int	stioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
int	stread __P((dev_t, struct uio *, int));
int	stwrite __P((dev_t, struct uio *, int));

void	ststrategy __P((struct buf *));
int	stdump __P((dev_t));

#ifdef DEBUG
void	dumpxsense __P((struct st_xsense *));
void	prtmodsel __P((struct mode_select_data *, int));
void	prtmodstat __P((struct mode_sense *));
#endif /* DEBUG */

static void	stfinish __P((struct st_softc *, struct buf *));
static void	sterror __P((struct st_softc *, int));
static int	stmatch __P((struct device *, void *, void *));
static void	stattach __P((struct device *, struct device *, void *));

struct cfattach st_ca = {
	sizeof(struct st_softc), stmatch, stattach
};

struct cfdriver st_cd = {
	NULL, "st", DV_TAPE
};

static int
stmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct oscsi_attach_args *osa = aux;

	if ((osa->osa_inqbuf->type != 0x01 ||	/* sequential access device */
	    osa->osa_inqbuf->qual != 0x80 ||	/* removable media */
	    (osa->osa_inqbuf->version != 0x01 &&
	     osa->osa_inqbuf->version != 0x02)) &&
	    (osa->osa_inqbuf->type != 0x01 ||	/* M4 ??! */
	     /*
	      * the M4 is a little too smart (ass?) for its own good:
	      * qual codes:
	      * 0x80: you can take the tape out (unit not online)
	      * 0xf8: online and at 6250bpi
	      * 0xf9: online and at 1600bpi
	      */
	     osa->osa_inqbuf->version != 0x09))	/* M4 tape */
		return (0);

	return (1);
}

static void
stattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct st_softc *sc = (struct st_softc *)self;
	struct oscsi_attach_args *osa = aux;
	char vendor[9], product[17], revision[5];
	int stat;
	static int havest = 0;

	printf("\n");

	sc->sc_tab.b_actb = &sc->sc_tab.b_actf;

	bzero(vendor, sizeof(vendor));
	bzero(product, sizeof(product));
	bzero(revision, sizeof(revision));

	scsi_str(osa->osa_inqbuf->vendor_id, vendor,
	    sizeof(osa->osa_inqbuf->vendor_id));
	scsi_str(osa->osa_inqbuf->product_id, product,
	    sizeof(osa->osa_inqbuf->product_id));
	scsi_str(osa->osa_inqbuf->rev, revision,
	    sizeof(osa->osa_inqbuf->rev));

	sc->sc_target = osa->osa_target;
	sc->sc_lun = osa->osa_lun;

	/* Initialize SCSI queue entry. */
	sc->sc_sq.sq_softc = sc;
	sc->sc_sq.sq_target = sc->sc_target;
	sc->sc_sq.sq_lun = sc->sc_lun;
	sc->sc_sq.sq_start = ststart;
	sc->sc_sq.sq_go = stgo;
	sc->sc_sq.sq_intr = stintr;

	if (bcmp("EXB-8200", product, 8) == 0) {
		sc->sc_tapeid = MT_ISEXABYTE;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 26;
		sc->sc_datalen[CMD_INQUIRY] = 52;
		sc->sc_datalen[CMD_MODE_SELECT] = 17;
		sc->sc_datalen[CMD_MODE_SENSE] = 17;
	} else if (bcmp("VIPER 150", product, 9) == 0 ||
		   bcmp("VIPER 60", product, 8) == 0) {
		sc->sc_tapeid = MT_ISVIPER1;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("Python 25501", product, 12) == 0 ||
		   bcmp("Python 28849", product, 12) == 0) {
		sc->sc_tapeid = MT_ISPYTHON;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("HP35450A", product, 8) == 0) {
		/* XXX "extra" stat makes the HP drive happy at boot time */
		stat = scsi_test_unit_rdy(sc->sc_dev.dv_parent->dv_unit,
		    sc->sc_target, sc->sc_lun);
		sc->sc_tapeid = MT_ISHPDAT;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 14;
		sc->sc_datalen[CMD_INQUIRY] = 36;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else if (bcmp("123107 SCSI", product, 11) == 0 ||
		   bcmp("OPEN REEL TAPE", product, 14) == 0) {
		sc->sc_tapeid = MT_ISMFOUR;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 8;
		sc->sc_datalen[CMD_INQUIRY] = 5;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	} else {
		printf("%s: Unsupported tape device, faking it\n",
		    sc->sc_dev.dv_xname);
		sc->sc_tapeid = MT_ISAR;
		sc->sc_datalen[CMD_REQUEST_SENSE] = 8;
		sc->sc_datalen[CMD_INQUIRY] = 5;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	}

	sc->sc_filepos = 0;
	
	/* load xsense */
	scsi_delay(-1);
	stxsense(sc->sc_dev.dv_parent->dv_unit, sc->sc_target, sc->sc_lun, sc);
	scsi_delay(0);

	/* XXX if we have a tape, we must up the delays in the HA driver */
	if (!havest) {
		havest = 1;
		scsi_delay(20000);
	}

	sc->sc_blkno = 0;
	sc->sc_flags = STF_ALIVE;
}

int
stopen(dev, flag, type, p)
	dev_t dev;
	int flag, type;
	struct proc *p;
{
	struct st_softc *sc;
	struct st_xsense *xsense;
	int count;
	int stat;
	int ctlr, slave, unit;
	struct mode_select_data msd;
	struct mode_sense mode;
	int modlen;
	int error;
	static struct scsi_fmt_cdb modsel = {
		6,
		{ CMD_MODE_SELECT, 0, 0, 0, sizeof(msd), 0 }
	};
	static struct scsi_fmt_cdb modsense = {
		6,
		{ CMD_MODE_SENSE, 0, 0, 0, sizeof(mode), 0 }
	};

	if (UNIT(dev) > st_cd.cd_ndevs ||
	    (sc = st_cd.cd_devs[UNIT(dev)]) == NULL ||
	    (sc->sc_flags & STF_ALIVE) == 0)
		return (ENXIO);

	if (sc->sc_flags & STF_OPEN)
		return (EBUSY);

	ctlr = sc->sc_dev.dv_parent->dv_unit;
	slave = sc->sc_target;
	unit = sc->sc_lun;
	xsense = &sc->sc_sense;

	/*
	 * Be prepared to print error messages
	 */
	sc->sc_ctty = tprintf_open(p);

	/* do a mode sense to get current */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = scsi_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);

	/* do a mode sense to get current */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = scsi_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);

	/* set record length */
	switch (sc->sc_tapeid) {
	case MT_ISAR:
		sc->sc_blklen = 512;
		break;
	case MT_ISEXABYTE:
		if (minor(dev) & STDEV_FIXEDBLK)
			sc->sc_blklen = 0x400;
		else
			sc->sc_blklen = st_exblklen;
		break;
	case MT_ISHPDAT:
		sc->sc_blklen = 512;
		break;
	case MT_ISVIPER1:
		sc->sc_blklen = 512;
		break;
	case MT_ISPYTHON:
		sc->sc_blklen = 512;
		break;
	case MT_ISMFOUR:
		sc->sc_blklen = 0;
		break;
	default:
		if ((mode.md.blklen2 << 16 |
		     mode.md.blklen1 << 8 |
		     mode.md.blklen0) != 0)
			sc->sc_blklen = mode.md.blklen2 << 16 |
					mode.md.blklen1 << 8 |
					mode.md.blklen0;
		else
			sc->sc_blklen = 512;
	}

	/* setup for mode select */
	msd.rsvd1 = 0;
	msd.rsvd2 = 0;
	msd.rsvd3 = 0;
	msd.buff = 1;
	msd.speed = 0;
	msd.blkdeslen = 0x08;
	msd.density = 0;
	msd.blks2 = 0;
	msd.blks1 = 0;
	msd.blks0 = 0;
	msd.rsvd4 = 0;
	msd.blklen2 = (sc->sc_blklen >> 16) & 0xff;
	msd.blklen1 = (sc->sc_blklen >> 8) & 0xff;
	msd.blklen0 = sc->sc_blklen & 0xff;

	/*
	 * Do we have any density problems?
	 */

	switch (sc->sc_tapeid) {
	case MT_ISAR:
		if (minor(dev) & STDEV_HIDENSITY)
			msd.density = 0x5;
		else {
			if (flag & FWRITE) {
				uprintf("Can only write QIC-24\n");
				return(EIO);
			}
			msd.density = 0x4;
		}
		break;
	case MT_ISEXABYTE:
		if (minor(dev) & STDEV_HIDENSITY)
			uprintf("EXB-8200 density support only\n");
		msd.vupb = (u_char)st_exvup;
		msd.rsvd5 = 0;
		msd.p5 = 0;
		msd.motionthres = (u_char)st_exmotthr;
		msd.reconthres = (u_char)st_exreconthr;
		msd.gapthres = (u_char)st_exgapthr;
		break;
	case MT_ISHPDAT:
	case MT_ISVIPER1:
	case MT_ISPYTHON:
		if (minor(dev) & STDEV_HIDENSITY)
			uprintf("Only one density supported\n");
		break;
	case MT_ISMFOUR:
		break;		/* XXX could do density select? */
	default:
		uprintf("Unsupported drive\n");
		return(EIO);
	}

	modlen = sc->sc_datalen[CMD_MODE_SELECT];
	modsel.cdb[4] = modlen;

	/* mode select */
	count = 0;
retryselect:
	stat = scsi_immed_command(ctlr, slave, unit, &modsel,
				  (u_char *)&msd, modlen, B_WRITE);
	/*
	 * First command after power cycle, bus reset or tape change 
	 * will error. Try command again
	 */
	if (stat == STS_CHECKCOND) {
		sc->sc_filepos = 0;
		stxsense(ctlr, slave, unit, sc);
		stat = scsi_immed_command(ctlr, slave, unit, &modsel,
					  (u_char *)&msd, modlen, B_WRITE);
#ifdef DEBUG
		if (stat && (st_debug & ST_OPEN))
			printf("stopen: stat on mode select 0x%x second try\n", stat);
#endif
		if (stat == STS_CHECKCOND) {
			stxsense(ctlr, slave, unit, sc);
			prtkey(sc);
		}
		if (stat)
			return(EIO);
	}
	if (stat == -1 || stat == STS_BUSY) {
		/*
		 * XXX it might just be that the bus is busy because
		 * another tape is doing a command. This would change
		 * with connect/disconnect, ie. the other drive would
		 * not hold onto the bus.
		 *
		 * Sleep on lbolt for up to 20 minutes (max time to FSF
		 * an exabyte to EOT: 16:xx minutes)
		 */
		if (++count > 60*20) {
			uprintf("SCSI bus timeout\n");
			return(EBUSY);
		}
		if ((error = tsleep((caddr_t)&lbolt, PZERO | PCATCH, 
		    "st_scsiwait", 0)))
			return (error);
		goto retryselect;
	}

	/* drive ready ? */
	stat = scsi_test_unit_rdy(ctlr, slave, unit);

	if (stat == STS_CHECKCOND) {
		stxsense(ctlr, slave, unit, sc);
		switch (sc->sc_tapeid) {
		case MT_ISEXABYTE:
			if ((xsense->sc_xsense.key & XSK_NOTRDY) &&
			    xsense->exb_xsense.tnp)
				uprintf("no cartridge\n");
			else if (xsense->sc_xsense.key & XSK_NOTRDY)
				uprintf("cartridge not loaded\n");
			else if (xsense->sc_xsense.key & XSK_UNTATTEN) {
				uprintf("new cart/power interrupt\n");
				stat = 0;
			} else if ((xsense->sc_xsense.key & XSK_UNTATTEN) &&
				   xsense->exb_xsense.tnp)
				uprintf("cartridge unloading\n");
			else 
				prtkey(sc);
			break;
		case MT_ISMFOUR:
		case MT_ISAR:
			if (xsense->sc_xsense.key & XSK_UNTATTEN)
				stat = scsi_test_unit_rdy(ctlr, slave, unit);
			if (stat == STS_CHECKCOND) {
				stxsense(ctlr, slave, unit, sc);
				if (xsense->sc_xsense.key)
					prtkey(sc);
			} else { 
				sc->sc_filepos = 0; /* new tape */
				stat = 0;
			}
			break;
		case MT_ISHPDAT:
		case MT_ISVIPER1:
		case MT_ISPYTHON:
			if (xsense->sc_xsense.key & XSK_UNTATTEN)
				stat = scsi_test_unit_rdy(ctlr, slave, unit);
			if (stat == STS_CHECKCOND) {
				stxsense(ctlr, slave, unit, sc);
				if (xsense->sc_xsense.key)
					prtkey(sc);
			}
			break;
		default:
			uprintf("%s: not ready\n", sc->sc_dev.dv_xname);
			prtkey(sc);
			break;
		}
	}
	if (stat)
		return(EIO);

	/* mode sense */
	modlen = sc->sc_datalen[CMD_MODE_SENSE];
	modsense.cdb[4] = modlen;
	stat = scsi_immed_command(ctlr, slave, unit, &modsense,
				  (u_char *)&mode, modlen, B_READ);
#ifdef DEBUG
	if (st_debug & ST_OPENSTAT)
		prtmodstat(&mode);
#endif

	if (stat == STS_CHECKCOND) {
		stxsense(ctlr, slave, unit, sc);
#ifdef DEBUG
		if (st_debug & ST_OPEN)
			dumpxsense(xsense);
#endif
	}
	if (stat)
		return(EIO);

	if ((flag & FWRITE) && mode.md.wp) {
		uprintf("st:%d write protected\n", UNIT(dev));
		return(EACCES);
	}

	/* save total number of blocks on tape */
	sc->sc_numblks = mode.md.numblk2 << 16 |
			 mode.md.numblk1 << 8 |
			 mode.md.numblk0;

	if (xsense->sc_xsense.eom && !(sc->sc_flags & STF_LEOT))
		sc->sc_filepos = 0;
#ifdef DEBUG
	if (st_debug & ST_FMKS)
		printf("%s: open filepos = %d\n", sc->sc_dev.dv_xname,
		    sc->sc_filepos);
#endif

	sc->sc_flags |= (STF_OPEN);
	if (flag & FWRITE)
		sc->sc_flags |= STF_WMODE;
	sc->sc_flags &= ~STF_MOVED;

#ifdef TTI
	/* make stats available, also lit up TTi display */
	sc->sc_tticntdwn = 100;
#endif
	stxsense(ctlr, slave, unit, sc);

	return(0);
}

/*ARGSUSED*/
int
stclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct st_softc *sc = st_cd.cd_devs[UNIT(dev)];
	int hit = 0;

	if ((sc->sc_flags & (STF_WMODE|STF_WRTTN)) == (STF_WMODE|STF_WRTTN)) {
		/*
		 * Cartridge tapes don't do double EOFs on EOT.
		 * We assume that variable-block devices use double EOF.
		 */
		stcommand(dev, MTWEOF, 1); 
		if (sc->sc_blklen == 0) {
			stcommand(dev, MTWEOF, 1); 
			stcommand(dev, MTBSR, 1); 
		}
		hit++;
	}
	if ((minor(dev) & STDEV_NOREWIND) == 0) {
		stcommand(dev, MTREW, 1);
		hit++;
	}
#ifdef NOTDEF
	/* wait until more stable before trying [XXX Needed ?] */
	if (!hit && (sc->sc_flags & SFT_WMODE))
		/* force out any any bufferd write data */
		stcommand(dev, MTFSR, 0); 
#endif
	/* make stats available */
	stxsense(sc->sc_dev.dv_parent->dv_unit, sc->sc_target, sc->sc_lun, sc);

	sc->sc_flags &= ~(STF_OPEN|STF_WMODE|STF_WRTTN);
	tprintf_close(sc->sc_ctty);
	return(0);	/* XXX */
}

void
ststrategy(bp)
	struct buf *bp;
{
	struct st_softc *sc;
	struct buf *dp;
	int unit, s;

	unit = UNIT(bp->b_dev);
	sc = st_cd.cd_devs[unit];

	dp = &sc->sc_tab;
	bp->b_actf = NULL;
	s = splbio();
	bp->b_actb = dp->b_actb;
	*dp->b_actb = bp;
	dp->b_actb = &bp->b_actf;
	if (dp->b_active == 0) {
		dp->b_active = 1;
		stustart(unit);
	}
	splx(s);
}

void
stustart(unit)
	int unit;
{
	struct st_softc *sc = st_cd.cd_devs[unit];

	if (scsireq(sc->sc_dev.dv_parent, &sc->sc_sq))
		ststart(sc);
}

void
ststart(arg)
	void *arg;
{
	struct st_softc *sc = arg;

	if (scsiustart(sc->sc_dev.dv_parent->dv_unit))
		stgo(arg);
}

void
stgo(arg)
	void *arg;
{
	struct st_softc *sc = arg;
	struct scsi_fmt_cdb *cmd;
	struct buf *bp = sc->sc_tab.b_actf;
	int pad, stat;
	long nblks;

	if (sc->sc_flags & STF_CMD) {
		cmd = &sc->sc_cmdstore;
		pad = 0;
	} else {
		cmd = bp->b_flags & B_READ ? &st_read_cmd : &st_write_cmd;
		if (sc->sc_blklen)
			cmd->cdb[1] |= 0x01; /* fixed mode */
		else
			cmd->cdb[1] &= 0xfe;
		if (bp->b_flags & B_READ)
			sc->sc_flags &= ~STF_WRTTN;
		else
			sc->sc_flags |= STF_WRTTN;

		if (sc->sc_blklen) { /* fixed mode */
			nblks = bp->b_bcount / sc->sc_blklen;
			if (bp->b_bcount % sc->sc_blklen) {
				tprintf(sc->sc_ctty,
					"%s: I/O not block aligned %d/%ld\n",
					sc->sc_dev.dv_xname, sc->sc_blklen,
					bp->b_bcount);
				cmd->cdb[1] &= 0xfe; /* force error */
			}
		} else	/* variable len */
			nblks = bp->b_bcount;

		*(u_char *)(&cmd->cdb[2]) = (u_char) (nblks >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (nblks >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) nblks;
		/*
		 * Always Zero. We are either writing in variable
		 * length mode we are writing in fixed block mode,
		 * or we are going to do odd length IO and are not
		 * going to use DMA.
		 */
		pad = 0; 
	}

#ifdef DEBUG
	if (st_debug & ST_GO)
		printf("stgo: cmd len %d [0]0x%x [1]0x%x [2]0x%x [3]0x%x [4]0x%x [5]0x%x\n",
		       cmd->len, cmd->cdb[0], cmd->cdb[1], cmd->cdb[2],
		       cmd->cdb[3], cmd->cdb[4], cmd->cdb[5]);
#endif

	sc->sc_flags |= STF_MOVED;
	if (bp->b_bcount & 1) {
#ifdef DEBUG
		if (st_debug & ST_ODDIO)
			printf("%s: stgo: odd count %ld using manual transfer\n",
			       sc->sc_dev.dv_xname, bp->b_bcount);
#endif
		stat = scsi_tt_oddio(sc->sc_dev.dv_parent->dv_unit,
		    sc->sc_target, sc->sc_lun, bp->b_un.b_addr, bp->b_bcount,
		    bp->b_flags, 1);
		if (stat == 0) {
			bp->b_resid = 0;
			stfinish(sc, bp);
		}
	} else
		stat = scsigo(sc->sc_dev.dv_parent->dv_unit,
		    sc->sc_target, sc->sc_lun, bp, cmd, pad);
	if (stat) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		stxsense(sc->sc_dev.dv_parent->dv_unit, sc->sc_target,
		    sc->sc_lun, sc);
		sterror(sc, stat);
		stfinish(sc, bp);
	}
}

static void
stfinish(sc, bp)
	struct st_softc *sc;
	struct buf *bp;
{
	struct buf *dp;

	sc->sc_tab.b_errcnt = 0;
	if ((dp = bp->b_actf))
		dp->b_actb = bp->b_actb;
	else
		sc->sc_tab.b_actb = bp->b_actb;
	*bp->b_actb = dp;
	iodone(bp);
	scsifree(sc->sc_dev.dv_parent, &sc->sc_sq);
	if (sc->sc_tab.b_actf)
		stustart(sc->sc_dev.dv_unit);
	else
		sc->sc_tab.b_active = 0;
}

int
stread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{

	return (physio(ststrategy, NULL, dev, B_READ, minphys, uio));
}

int
stwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{

	/* XXX: check for hardware write-protect? */
	return (physio(ststrategy, NULL, dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
int
stdump(dev)
	dev_t dev;
{
	return(ENXIO);
}

/*ARGSUSED*/
int
stioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data; 
	int flag;
	struct proc *p;
{
	struct st_softc *sc = st_cd.cd_devs[UNIT(dev)];
	int cnt;
	struct mtget *mtget;
	struct st_xsense *xp = &sc->sc_sense;
	struct mtop *op;
	long resid = 0;	/* XXX compiler complains needlessly :-( */

	switch (cmd) {

	/* tape operation */
	case MTIOCTOP:
		op = (struct mtop *)data;
		switch (op->mt_op) {

		case MTBSR:
		case MTFSR:
			if (sc->sc_tapeid == MT_ISAR)
				return(ENXIO);
		case MTWEOF:
		case MTFSF:
		case MTBSF:
			cnt = (int)op->mt_count;
			break;

		case MTREW:
		case MTOFFL:
			cnt = 1;
			break;

		case MTNOP:
			return(0);
		default:
			return(EINVAL);
		}
		if (cnt <= 0)
			return(EINVAL);
		stcommand(dev, (u_int)op->mt_op, cnt);
		break;

	/* drive status */
	case MTIOCGET:
		mtget = (struct mtget *)data;
		stxsense(sc->sc_dev.dv_parent->dv_unit, sc->sc_target,
		    sc->sc_lun, sc);
		mtget->mt_type = sc->sc_tapeid;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = ((xp->sc_xsense.valid << 15) |
				   (xp->sc_xsense.filemark << 14) |
				   (xp->sc_xsense.eom << 13) |
				   (xp->sc_xsense.ili << 12) |
				   (xp->sc_xsense.key << 8));
		
		if (sc->sc_tapeid == MT_ISEXABYTE) {
			mtget->mt_dsreg = sc->sc_flags;
			resid = (xp->exb_xsense.tplft2 << 16 |
				 xp->exb_xsense.tplft1 << 8 |
				 xp->exb_xsense.tplft0);
			mtget->mt_resid = resid / 1000;
			mtget->mt_erreg |= (((xp->exb_xsense.rwerrcnt2 << 16 |
					      xp->exb_xsense.rwerrcnt1 << 8 |
					      xp->exb_xsense.rwerrcnt0) * 100) / 
					    (sc->sc_numblks - resid)) & 0xff;
		} else if (xp->sc_xsense.valid) {
			resid = ((xp->sc_xsense.info1 << 24) |
				 (xp->sc_xsense.info2 << 16) |
				 (xp->sc_xsense.info3 << 8) |
				 (xp->sc_xsense.info4));
			if (sc->sc_blklen) /* if fixed mode */
				resid *= sc->sc_blklen;
			mtget->mt_resid = resid;
		} else
			mtget->mt_resid = 0;
		break;

	default:
		return(ENXIO);
	}
	return(0);
}

void
stintr(arg, stat)
	void *arg;
	int stat;
{
	struct st_softc *sc = arg;
	struct st_xsense *xp = &sc->sc_sense;
	struct buf *bp = sc->sc_tab.b_actf;

#ifdef DEBUG
	if (bp == NULL) {
		printf("%s: bp == NULL\n", sc->sc_dev.dv_xname);
		return;
	}
#endif
	switch (stat) {
	/* scsi command completed ok */
	case 0:
		bp->b_resid = 0;
		break;

	/* more status */
	case STS_CHECKCOND:
		stxsense(sc->sc_dev.dv_parent->dv_unit, sc->sc_target,
		    sc->sc_lun, sc);
		if (xp->sc_xsense.valid) {
			bp->b_resid = (u_long)((xp->sc_xsense.info1 << 24) |
					      (xp->sc_xsense.info2 << 16) |
					      (xp->sc_xsense.info3 << 8) |
					      (xp->sc_xsense.info4));
			if (sc->sc_blklen) /* fixed mode */
				bp->b_resid *= sc->sc_blklen;
		}
		if (xp->sc_xsense.filemark) {
			sc->sc_filepos++;
			break;
		}
		if (xp->sc_xsense.key != XSK_NOSENCE
		    && xp->sc_xsense.key != XSK_NOTUSED1
		    && xp->sc_xsense.key != XSK_NOTUSEDC
		    && xp->sc_xsense.key != XSK_NOTUSEDE) {
			sterror(sc, stat);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			break;
		}
		if (xp->sc_xsense.ili) {
			/*
			 * Fixed length blocks, an error.
			 */
			if (sc->sc_blklen) {
				tprintf(sc->sc_ctty,
					"%s: Incorrect Length Indicator, blkcnt diff %ld\n",
					sc->sc_dev.dv_xname,
					sc->sc_blklen - bp->b_resid);
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				break;
			}
			/*
			 * Variable length but read more than requested,
			 * an error.  (XXX ??? wrong for 9 track?)
			 */
			if (bp->b_resid < 0) {
				bp->b_resid = 0;
				bp->b_flags |= B_ERROR;
				bp->b_error = ENOMEM;
				break;
			}
			/*
			 * Variable length and odd, may require special
			 * handling.
			 */
			if (bp->b_resid & 1 && (sc->sc_tapeid != MT_ISAR)) {
				/*
				 * Normal behavior, treat as an error.
				 */
				if (!st_dmaoddretry) {
					tprintf(sc->sc_ctty,
						"%s: Odd length read %ld\n", 
						sc->sc_dev.dv_xname,
						bp->b_bcount - bp->b_resid);
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;
					break;
				}
				/*
				 * Attempt to back up and re-read using oddio.
				 */
#ifdef DEBUG
				if (st_debug & ST_ODDIO)
					printf("%s: stintr odd count %ld, do BSR then oddio\n",
					       sc->sc_dev.dv_xname,
					       bp->b_bcount - bp->b_resid);
#endif
				stat =
				  scsi_tt_oddio(sc->sc_dev.dv_parent->dv_unit,
				  sc->sc_target, sc->sc_lun, 0, -1, 0, 0);
				if (stat == 0)
					stat = scsi_tt_oddio(
					    sc->sc_dev.dv_parent->dv_unit,
					    sc->sc_target, sc->sc_lun,
					    bp->b_un.b_addr,
					    bp->b_bcount - bp->b_resid,
					    bp->b_flags, 0);
				if (stat) {
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;
					stxsense(sc->sc_dev.dv_parent->dv_unit,
					    sc->sc_target, sc->sc_lun, sc);
					sterror(sc, stat);
				}
			}
			break;
		}
		if (xp->sc_xsense.eom) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENOSPC;
			break;
		}
		tprintf(sc->sc_ctty, "%s: unknown scsi error\n",
		    sc->sc_dev.dv_xname);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		break;

	default:
		printf("%s: stintr unknown stat 0x%x\n", sc->sc_dev.dv_xname,
		    stat);
		break;
	}
#ifdef DEBUG
	if ((st_debug & ST_BRESID) && bp->b_resid != 0)
		printf("b_resid %u b_flags 0x%lx b_error 0x%x\n", 
		       bp->b_resid, bp->b_flags, bp->b_error);
#endif
	/* asked for more filemarks then on tape */
	if (bp->b_resid != 0 &&
	    (sc->sc_flags & STF_CMD) && sc->sc_cmd == CMD_SPACE) {
		sc->sc_filepos -= bp->b_resid;
		if (sc->sc_filepos < 0)
			sc->sc_filepos = 0;
	}

#ifdef TTI
	if (st_extti & (1 << sc->sc_dev.dv_unit) &&
	    sc->sc_type == MT_ISEXABYTE) /* to make display lit up */
		/*
		 * XXX severe performance penality for this.
		 * Try and throttle by not calling stxsense on every intr.
		 * Mostly for TTi we, get a stxsense call in open and close.
		 */
		if (sc->sc_tticntdwn-- == 0) {
			stxsense(sc->sc_dev.dv_parent->dv_unit,
			    sc->sc_target, sc->sc_lun, sc);
			sc->sc_tticntdwn = 100;
		}
#endif

	stfinish(sc, bp);
}

void
stcommand(dev, command, cnt)
	dev_t dev;
	u_int command;
	int cnt;
{
	struct st_softc *sc = st_cd.cd_devs[UNIT(dev)];
	struct buf *bp = &sc->sc_bufstore;
	struct scsi_fmt_cdb *cmd = &sc->sc_cmdstore;
	int cmdcnt, s;

	cmd->len = 6; /* all tape commands are cdb6 */
	cmd->cdb[1] = sc->sc_lun;
	cmd->cdb[2] = cmd->cdb[3] = cmd->cdb[4] = cmd->cdb[5] = 0;
	cmdcnt = 0;

	/*
	 * XXX Assumption is that everything except Archive can take
	 * repeat count in cdb block.
	 */
	switch (command) {
	case MTWEOF:
		cmd->cdb[0] = CMD_WRITE_FILEMARK;
		if (sc->sc_tapeid != MT_ISAR) {
			cmdcnt = cnt;
			cnt = 1;
		} else
			cmdcnt = 1;
		*(u_char *)(&cmd->cdb[2]) = (u_char) (cmdcnt >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (cmdcnt >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) cmdcnt;

		if (sc->sc_tapeid == MT_ISEXABYTE &&
		    (minor(dev) & STDEV_EXSFMK)) /* short filemarks */
			cmd->cdb[5] |= 0x80;
		else
			cmd->cdb[5] &= 0x7f;
		break;
	case MTBSF:
		/* Archive can't back up, will not get to BSR case */
		if (sc->sc_tapeid == MT_ISAR) {
			if ((sc->sc_filepos - cnt) < 0) {
				stcommand(dev, MTREW, 1);
				return;
			}
			cmdcnt = sc->sc_filepos - cnt + 1;
			stcommand(dev, MTREW, 1);
			stcommand(dev, MTFSF, cmdcnt);
			return;
		}
	case MTBSR:
	case MTFSR:
	case MTFSF:
		if (command == MTBSF || command == MTBSR)
			cnt = cnt * (-1); /* backward move */
		if (command == MTFSF || command == MTBSF)
			cmd->cdb[1] |= 0x01; /* filemarks */
		else
			cmd->cdb[1] |= 0x00; /* logical blocks */
		if (sc->sc_tapeid != MT_ISAR) {
			cmdcnt = cnt;
			cnt = 1;
		} else
			cmdcnt = 1;
		*(u_char *)(&cmd->cdb[2]) = (u_char) (cmdcnt >> 16);
		*(u_char *)(&cmd->cdb[3]) = (u_char) (cmdcnt >> 8);
		*(u_char *)(&cmd->cdb[4]) = (u_char) cmdcnt;
		cmd->cdb[0] = CMD_SPACE;
		break;
	case MTREW:
		cmd->cdb[0] = CMD_REWIND;
		sc->sc_filepos = 0;
		break;
	case MTOFFL:
		cmd->cdb[0] = CMD_LOADUNLOAD;
		sc->sc_filepos = 0;
		break;
	default:
		printf("%s: stcommand bad command 0x%x\n", 
		       sc->sc_dev.dv_xname, command);
	}

	sc->sc_flags |= STF_CMD;
	sc->sc_cmd = cmd->cdb[0];

	sc->sc_bp = bp;
again:
#ifdef DEBUG
	if (st_debug & ST_FMKS)
		printf("%s: stcommand filepos %d cmdcnt %d cnt %d\n", 
		       sc->sc_dev.dv_xname, sc->sc_filepos, cmdcnt, cnt);
#endif
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		if (bp->b_flags & B_DONE)
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_bcount = 0;
	bp->b_resid = 0;
	bp->b_blkno = 0;
	bp->b_error = 0;
	ststrategy(bp);
	iowait(bp);
	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;

	if (command == MTWEOF || command == MTFSF || command == MTBSF)
		sc->sc_filepos += cmdcnt;

	if (--cnt > 0)
		goto again;

	sc->sc_flags |= STF_MOVED;
	sc->sc_flags &= ~(STF_CMD|STF_WRTTN);
}

static void
sterror(sc, stat)
	struct st_softc *sc;
	int stat;
{
	/* stxsense must have been called before sterror() */
	if (stat & STS_CHECKCOND)
		prtkey(sc);
	else if (stat)
		tprintf(sc->sc_ctty,
			"%s: bad scsi status 0x%x\n", sc->sc_dev.dv_xname,
			    stat);

	if ((sc->sc_flags & STF_CMD) && sc->sc_cmd == CMD_SPACE) /* fsf */
		sc->sc_filepos--;
}

void
stxsense(ctlr, slave, unit, sc)
	int ctlr, slave, unit;
	struct st_softc *sc;
{
	unsigned len;

	len = sc->sc_datalen[CMD_REQUEST_SENSE];
	scsi_request_sense(ctlr, slave, unit, (u_char *)&sc->sc_sense, len);
}

void
prtkey(sc)
	struct st_softc *sc;
{
	struct st_xsense *xp = &sc->sc_sense;

	switch (xp->sc_xsense.key) {
	case XSK_NOSENCE:
		break;
	case XSK_NOTUSED1:
	case XSK_NOTUSEDC:
	case XSK_NOTUSEDE:
		break;
	case XSK_REVERVED:
		tprintf(sc->sc_ctty, "%s: Reserved sense key 0x%x\n",
			sc->sc_dev.dv_xname, xp->sc_xsense.key);
		break;
	case XSK_NOTRDY:
		tprintf(sc->sc_ctty, "%s: NOT READY\n", sc->sc_dev.dv_xname);
		break;
	case XSK_MEDERR:
		tprintf(sc->sc_ctty, "%s: MEDIUM ERROR\n", sc->sc_dev.dv_xname);
		break;
	case XSK_HRDWERR:
		tprintf(sc->sc_ctty, "%s: HARDWARE ERROR\n",
		    sc->sc_dev.dv_xname);
		break;
	case XSK_ILLREQ:
		tprintf(sc->sc_ctty, "%s: ILLEGAL REQUEST\n",
		    sc->sc_dev.dv_xname);
		break;
	case XSK_UNTATTEN:
		tprintf(sc->sc_ctty, "%s: UNIT ATTENTION\n",
		    sc->sc_dev.dv_xname);
		break;
	case XSK_DATAPROT:
		tprintf(sc->sc_ctty, "%s: DATA PROTECT\n", sc->sc_dev.dv_xname);
		break;
	case XSK_BLNKCHK:
		tprintf(sc->sc_ctty, "%s: BLANK CHECK\n", sc->sc_dev.dv_xname);
		break;
	case XSK_VENDOR:
		tprintf(sc->sc_ctty, "%s: VENDER UNIQUE SENSE KEY ",
		    sc->sc_dev.dv_xname);
		switch (sc->sc_tapeid) {
		case MT_ISEXABYTE:
			tprintf(sc->sc_ctty, "Exabyte: ");
			if (xp->exb_xsense.xfr)
				tprintf(sc->sc_ctty,
					"Transfer Abort Error\n");
			if (xp->exb_xsense.tmd)
				tprintf(sc->sc_ctty,
					"Tape Mark Detect Error\n");
			break;
		default:
			tprintf(sc->sc_ctty, "\n");
		}
		break;
	case XSK_CPYABORT:
		tprintf(sc->sc_ctty, "%s: COPY ABORTED\n", sc->sc_dev.dv_xname);
		break;
	case XSK_ABORTCMD:
		tprintf(sc->sc_ctty, "%s: ABORTED COMMAND\n",
		    sc->sc_dev.dv_xname);
		break;
	case XSK_VOLOVER:
		tprintf(sc->sc_ctty, "%s: VOLUME OVERFLOW\n",
		    sc->sc_dev.dv_xname);
		break;
	default:
		tprintf(sc->sc_ctty, "%s: unknown sense key 0x%x\n",
			sc->sc_dev.dv_xname, xp->sc_xsense.key);
	}
	if (sc->sc_tapeid == MT_ISEXABYTE) {
		if (xp->exb_xsense.bpe)
			tprintf(sc->sc_ctty, "%s: Bus Parity Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.fpe)
			tprintf(sc->sc_ctty,
				"%s: Formatted Buffer Parity Error",
				sc->sc_dev.dv_xname);
		if (xp->exb_xsense.eco)
			tprintf(sc->sc_ctty, "%s: Error Counter Overflow",
				sc->sc_dev.dv_xname);
		if (xp->exb_xsense.tme)
			tprintf(sc->sc_ctty, "%s: Tape Motion Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.xfr)
			tprintf(sc->sc_ctty, "%s: Transfer About Error",
				sc->sc_dev.dv_xname);
		if (xp->exb_xsense.tmd)
			tprintf(sc->sc_ctty, "%s: Tape Mark Detect Error",
				sc->sc_dev.dv_xname);
		if (xp->exb_xsense.fmke)
			tprintf(sc->sc_ctty, "%s: Filemark Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.ure)
			tprintf(sc->sc_ctty, "%s: Under Run Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.sse)
			tprintf(sc->sc_ctty, "%s: Servo System Error",
				sc->sc_dev.dv_xname);
		if (xp->exb_xsense.fe)
			tprintf(sc->sc_ctty, "%s: Formatter Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.wseb)
			tprintf(sc->sc_ctty, "%s: WSEB Error",
			    sc->sc_dev.dv_xname);
		if (xp->exb_xsense.wseo)
			tprintf(sc->sc_ctty, "%s: WSEO Error",
			    sc->sc_dev.dv_xname);
	}
}

#ifdef DEBUG
void
dumpxsense(sensebuf)
	struct st_xsense *sensebuf;
{
        struct st_xsense *xp = sensebuf;

	printf("valid 0x%x errorclass 0x%x errorcode 0x%x\n", 
	       xp->sc_xsense.valid, 
	       xp->sc_xsense.class, xp->sc_xsense.code);
	printf("seg number 0x%x\n", xp->sc_xsense.segment);
	printf("FMK 0x%x EOM 0x%x ILI 0x%x RSVD 0x%x sensekey 0x%x\n", 
	       xp->sc_xsense.filemark, xp->sc_xsense.eom, xp->sc_xsense.ili, 
	       xp->sc_xsense.rsvd, xp->sc_xsense.key);
	printf("info 0x%lx\n", 
	       (u_long)((xp->sc_xsense.info1<<24)|(xp->sc_xsense.info2<<16)|
			(xp->sc_xsense.info3<<8)|(xp->sc_xsense.info4)) );
	printf("ASenseL 0x%x\n", xp->sc_xsense.len);

	if (xp->sc_xsense.len != 0x12) /* MT_ISEXB Exabyte only ?? */
		return;			/* What about others */

	printf("ASenseC 0x%x\n", xp->exb_xsense.addsens);
	printf("AsenseQ 0x%x\n", xp->exb_xsense.addsensq);
	printf("R/W Errors 0x%lx\n", 
	       (u_long)((xp->exb_xsense.rwerrcnt2<<16)|
			(xp->exb_xsense.rwerrcnt1<<8)|
			(xp->exb_xsense.rwerrcnt1)) );
	printf("PF   0x%x BPE  0x%x FPE 0x%x ME   0x%x ECO 0x%x TME 0x%x TNP 0x%x BOT 0x%x\n",
	       xp->exb_xsense.pf, xp->exb_xsense.bpe, xp->exb_xsense.fpe, 
	       xp->exb_xsense.me, xp->exb_xsense.eco, xp->exb_xsense.tme, 
	       xp->exb_xsense.tnp, xp->exb_xsense.bot);
	printf("XFR  0x%x TMD  0x%x WP  0x%x FMKE 0x%x URE 0x%x WE1 0x%x SSE 0x%x FE  0x%x\n",
	       xp->exb_xsense.xfr, xp->exb_xsense.tmd, xp->exb_xsense.wp, 
	       xp->exb_xsense.fmke, xp->exb_xsense.ure, xp->exb_xsense.we1, 
	       xp->exb_xsense.sse, xp->exb_xsense.fe);
	printf("WSEB 0x%x WSEO 0x%x\n",
	       xp->exb_xsense.wseb, xp->exb_xsense.wseo);
	printf("Remaining Tape 0x%lx\n", 
	       (u_long)((xp->exb_xsense.tplft2<<16)|
			(xp->exb_xsense.tplft1<<8)|
			(xp->exb_xsense.tplft0)) );
}

void
prtmodsel(msd, modlen)
	struct mode_select_data *msd;
	int modlen;
{
	printf("Modsel command. len is 0x%x.\n", modlen);
	printf("rsvd1 0x%x rsvd2 0x%x rsvd3 0x%x bufferd 0x%x speed 0x%x bckdeslen 0x%x\n",
	       msd->rsvd1,msd->rsvd2,msd->rsvd3,msd->buff,msd->speed,msd->blkdeslen);
	printf("density 0x%x blks2 0x%x blks1 0x%x blks0 0x%x rsvd 0x%x blklen2 0x%x blklen1 0x%x blklen0 0x%x\n",
	       msd->density,msd->blks2,msd->blks1,msd->blks0,msd->rsvd4,msd->blklen2,msd->blklen1,msd->blklen0);
	printf("vupb 0x%x rsvd 0x%x p5 0x%x motionthres 0x%x reconthres 0x%x gapthres 0x%x \n",
	       msd->vupb,msd->rsvd5,msd->p5,msd->motionthres,msd->reconthres,msd->gapthres);
}

void
prtmodstat(mode)
	struct mode_sense *mode;
{
	printf("Mode Status\n");
	printf("sdl 0x%x medtype 0x%x wp 0x%x bfmd 0x%x speed 0x%x bdl 0x%x\n",
	       mode->md.sdl, mode->md.medtype, mode->md.wp, mode->md.bfmd, 
	       mode->md.speed, mode->md.bdl);
	printf("dencod 0x%x numblk 0x%x blklen 0x%x\n",
	       mode->md.dencod, 
	       (mode->md.numblk2<<16)|(mode->md.numblk1<<8)|(mode->md.numblk0),
	       (mode->md.blklen2<<16)|(mode->md.blklen1<<8)|(mode->md.blklen0) );
	printf("ct 0x%x nd 0x%x nbe 0x%x edb 0x%x pe 0x%x nal 0x%x p5 0x%x\n",
	       mode->ex.ct, mode->ex.nd, mode->ex.nbe, 
	       mode->ex.ebd, mode->ex.pe, mode->ex.nal, mode->ex.p5);
	printf("motionthres 0x%x reconthres 0x%x gapthres 0x%x\n",
	       mode->ex.motionthres, mode->ex.reconthres,  mode->ex.gapthres);
}
#endif /* DEBUG */

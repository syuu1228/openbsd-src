/*	$OpenBSD: src/sys/dev/ic/com.c,v 1.9 1996/03/20 01:00:42 mickey Exp $	*/
/*	$NetBSD: com.c,v 1.65 1996/02/10 20:23:18 christos Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

/*
 * COM driver, based on HP dca driver
 * uses National Semiconductor NS16450/NS16550AF UART
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/ic/ns16550reg.h>
#ifdef COM_HAYESP
#include <dev/ic/hayespreg.h>
#endif
#define	com_lcr	com_cfcr

#define	COM_IBUFSIZE	(2 * 512)
#define	COM_IHIGHWATER	((3 * COM_IBUFSIZE) / 4)

struct com_softc {
	struct device sc_dev;
	void *sc_ih;
	struct tty *sc_tty;

	int sc_overflows;
	int sc_floods;
	int sc_errors;

	int sc_iobase;
#ifdef COM_HAYESP
	int sc_hayespbase;
#endif
	u_char sc_hwflags;
#define	COM_HW_NOIEN	0x01
#define	COM_HW_FIFO	0x02
#define	COM_HW_HAYESP	0x04
#define	COM_HW_CONSOLE	0x40
	u_char sc_swflags;
#define	COM_SW_SOFTCAR	0x01
#define	COM_SW_CLOCAL	0x02
#define	COM_SW_CRTSCTS	0x04
#define	COM_SW_MDMBUF	0x08
	u_char sc_msr, sc_mcr, sc_lcr;
	u_char sc_dtr;

	u_char *sc_ibuf, *sc_ibufp, *sc_ibufhigh, *sc_ibufend;
	u_char sc_ibufs[2][COM_IBUFSIZE];
};

int comprobe __P((struct device *, void *, void *));
void comattach __P((struct device *, struct device *, void *));
int comopen __P((dev_t, int, int, struct proc *));
int comclose __P((dev_t, int, int, struct proc *));
void comdiag __P((void *));
int comintr __P((void *));
void compoll __P((void *));
int comparam __P((struct tty *, struct termios *));
void comstart __P((struct tty *));

struct cfdriver comcd = {
	NULL, "com", comprobe, comattach, DV_TTY, sizeof(struct com_softc)
};

int	comdefaultrate = TTYDEF_SPEED;
#ifdef COMCONSOLE
int	comconsole = COMCONSOLE;
#else
int	comconsole = -1;
#endif
int	comconsinit;
int	commajor;
int	comsopen = 0;
int	comevents = 0;

#ifdef KGDB
#include <machine/remote-sl.h>
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

#define	COMUNIT(x)	(minor(x))

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

#include "pcmciabus.h"
#if NPCMCIABUS >0                       
/* additional setup needed for pcmcia devices */
#include <dev/pcmcia/pcmciabus.h>
/* modify config entry */
static int 
commod(pc_link,self,pc_cf,cf)
    struct pcmcia_link *pc_link;
    struct device *self;
    struct pcmcia_conf *pc_cf; 
    struct cfdata *cf;
{               
    int err; 
    struct pcmciadevs *dev=pc_link->device;
    struct ed_softc *sc = (void *)self; 
    if(!(err=pc_link->adapter->bus_link->bus_config(pc_link,self,pc_cf,cf))) {
        pc_cf->memwin=0;
	if(pc_cf->cfgtype==0) 
	    pc_cf->cfgtype=CFGENTRYID; /* determine from ioaddr */
    }
    return err;
}
static struct pcmcia_com {
    struct pcmcia_device pcd;
} pcmcia_com= {
    "PCMCIA Modem card",commod,NULL,NULL,NULL
};          
struct pcmciadevs pcmcia_com_devs[]={
  { "com", 0, 
  NULL, "*MODEM*", NULL, NULL,
  NULL, (void *)&pcmcia_com 
  },
  { "com", 0, 
  NULL, NULL, "*MODEM*", NULL,
  NULL, (void *)&pcmcia_com 
  },
  { "com", 0, 
  NULL, NULL, NULL, "*MODEM*",
  NULL, (void *)&pcmcia_com 
  },
  {NULL}
};
#endif

int
comspeed(speed)
	long speed;
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;

	if (speed == 0)
		return 0;
	if (speed < 0)
		return -1;
	x = divrnd((COM_FREQ / 16), speed);
	if (x <= 0)
		return -1;
	err = divrnd((COM_FREQ / 16) * 1000, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return -1;
	return x;

#undef	divrnd(n, q)
}

int
comprobe1(iobase)
	int iobase;
{
	int tmp;
	int i,k;

	/* force access to id reg */
	outb(iobase + com_lcr, 0);
	outb(iobase + com_iir, 0);
	for(i=0;i<32;i++) {
	    k=inb(iobase + com_iir);
	    if (k & 0x38) {
		inb(iobase + com_data ); /* cleanup */
	    } else
		break;
	}
	if(i>=32) 
	    return 0;

	return 1;
}

#ifdef COM_HAYESP
int
comprobeHAYESP(iobase, sc)
	int iobase;
	struct com_softc *sc;
{
	char	val, dips;
	int	combaselist[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };

	/*
	 * Hayes ESP cards have two iobases.  One is for compatibility with
	 * 16550 serial chips, and at the same ISA PC base addresses.  The
	 * other is for ESP-specific enhanced features, and lies at a
	 * different addressing range entirely (0x140, 0x180, 0x280, or 0x300).
	 */

	/* Test for ESP signature */
	if ((inb(iobase) & 0xf3) == 0)
		return 0;

	/*
	 * ESP is present at ESP enhanced base address; unknown com port
	 */

	/* Get the dip-switch configurations */
	outb(iobase + HAYESP_CMD1, HAYESP_GETDIPS);
	dips = inb(iobase + HAYESP_STATUS1);

	/* Determine which com port this ESP card services: bits 0,1 of  */
	/*  dips is the port # (0-3); combaselist[val] is the com_iobase */
	if (sc->sc_iobase != combaselist[dips & 0x03])
		return 0;

	printf(": ESP");

 	/* Check ESP Self Test bits. */
	/* Check for ESP version 2.0: bits 4,5,6 == 010 */
	outb(iobase + HAYESP_CMD1, HAYESP_GETTEST);
	val = inb(iobase + HAYESP_STATUS1);	/* Clear reg 1 */
	val = inb(iobase + HAYESP_STATUS2);
	if ((val & 0x70) < 0x20) {
		printf("-old (%o)", val & 0x70);
		/* we do not support the necessary features */
		return 0;
	}

	/* Check for ability to emulate 16550: bit 8 == 1 */
	if ((dips & 0x80) == 0) {
		printf(" slave");
		/* XXX Does slave really mean no 16550 support?? */
		return 0;
	}

	/*
	 * If we made it this far, we are a full-featured ESP v2.0 (or
	 * better), at the correct com port address.
	 */

	SET(sc->sc_hwflags, COM_HW_HAYESP);
	printf(", 1024k fifo\n");
	return 1;
}
#endif

int
comprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct isa_attach_args *ia = aux;
	int iobase = ia->ia_iobase;

	if (!comprobe1(iobase))
		return 0;

	ia->ia_iosize = COM_NPORTS;
	ia->ia_msize = 0;
	return 1;
}

void
comattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct cfdata *cf = sc->sc_dev.dv_cfdata;
	int iobase = ia->ia_iobase;
	struct tty *tp;
#ifdef COM_HAYESP
	int	hayesp_ports[] = { 0x140, 0x180, 0x280, 0x300, 0 };
	int	*hayespp;
#endif

	sc->sc_iobase = iobase;
	sc->sc_hwflags = ISSET(cf->cf_flags, COM_HW_NOIEN);
	sc->sc_swflags = 0;

	if (sc->sc_dev.dv_unit == comconsole)
		delay(1000);

#ifdef COM_HAYESP
	/* Look for a Hayes ESP board. */
	for (hayespp = hayesp_ports; *hayespp != 0; hayespp++)
		if (comprobeHAYESP(*hayespp, sc)) {
			sc->sc_hayespbase = *hayespp;
			break;
		}
	/* No ESP; look for other things. */
	if (*hayespp == 0) {
#endif

	/* look for a NS 16550AF UART with FIFOs */
	outb(iobase + com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_14);
	delay(100);
	if (ISSET(inb(iobase + com_iir), IIR_FIFO_MASK) == IIR_FIFO_MASK)
		if (ISSET(inb(iobase + com_fifo), FIFO_TRIGGER_14) == FIFO_TRIGGER_14) {
			SET(sc->sc_hwflags, COM_HW_FIFO);
			printf(": ns16550a, working fifo\n");
		} else
			printf(": ns16550, broken fifo\n");
	else
		printf(": ns8250 or ns16450, no fifo\n");
	outb(iobase + com_fifo, 0);
#ifdef COM_HAYESP
	}
#endif

	/* disable interrupts */
	outb(iobase + com_ier, 0);
	outb(iobase + com_mcr, 0);

	if (ia->ia_irq != IRQUNK)
		sc->sc_ih = isa_intr_establish(ia->ia_irq, IST_EDGE, IPL_TTY,
		    comintr, sc, sc->sc_dev.dv_xname);

#ifdef KGDB
	if (kgdb_dev == makedev(commajor, unit)) {
		if (comconsole == unit)
			kgdb_dev = -1;	/* can't debug over console port */
		else {
			(void) cominit(unit, kgdb_rate);
			if (kgdb_debug_init) {
				/*
				 * Print prefix of device name,
				 * let kgdb_connect print the rest.
				 */
				printf("%s: ", sc->sc_dev.dv_xname);
				kgdb_connect(1);
			} else
				printf("%s: kgdb enabled\n",
				    sc->sc_dev.dv_xname);
		}
	}
#endif

	if (sc->sc_dev.dv_unit == comconsole) {
		/*
		 * Need to reset baud rate, etc. of next print so reset
		 * comconsinit.  Also make sure console is always "hardwired".
		 */
		comconsinit = 0;
		SET(sc->sc_hwflags, COM_HW_CONSOLE);
		SET(sc->sc_swflags, COM_SW_SOFTCAR);
	}
}

int
comopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc;
	int iobase;
	struct tty *tp;
	int s;
	int error = 0;
 
	if (unit >= comcd.cd_ndevs)
		return ENXIO;
	sc = comcd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (!sc->sc_tty)
		tp = sc->sc_tty = ttymalloc();
	else
		tp = sc->sc_tty;

	tp->t_oproc = comstart;
	tp->t_param = comparam;
	tp->t_dev = dev;
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		SET(tp->t_state, TS_WOPEN);
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		if (ISSET(sc->sc_swflags, COM_SW_CLOCAL))
			SET(tp->t_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, COM_SW_CRTSCTS))
			SET(tp->t_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, COM_SW_MDMBUF))
			SET(tp->t_cflag, MDMBUF);
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = comdefaultrate;

		s = spltty();

		comparam(tp, &tp->t_termios);
		ttsetwater(tp);

		if (comsopen++ == 0)
			timeout(compoll, NULL, 1);

		sc->sc_ibufp = sc->sc_ibuf = sc->sc_ibufs[0];
		sc->sc_ibufhigh = sc->sc_ibuf + COM_IHIGHWATER;
		sc->sc_ibufend = sc->sc_ibuf + COM_IBUFSIZE;

		iobase = sc->sc_iobase;
#ifdef COM_HAYESP
		/* Setup the ESP board */
		if (ISSET(sc->sc_hwflags, COM_HW_HAYESP)) {
			int hayespbase = sc->sc_hayespbase;

			outb(iobase + com_fifo,
			     FIFO_DMA_MODE|FIFO_ENABLE|
			     FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_8);

			/* Set 16550 compatibility mode */
			outb(hayespbase + HAYESP_CMD1, HAYESP_SETMODE);
			outb(hayespbase + HAYESP_CMD2, 
			     HAYESP_MODE_FIFO|HAYESP_MODE_RTS|
			     HAYESP_MODE_SCALE);

			/* Set RTS/CTS flow control */
			outb(hayespbase + HAYESP_CMD1, HAYESP_SETFLOWTYPE);
			outb(hayespbase + HAYESP_CMD2, HAYESP_FLOW_RTS);
			outb(hayespbase + HAYESP_CMD2, HAYESP_FLOW_CTS);

			/* Set flow control levels */
			outb(hayespbase + HAYESP_CMD1, HAYESP_SETRXFLOW);
			outb(hayespbase + HAYESP_CMD2, 
			     HAYESP_HIBYTE(HAYESP_RXHIWMARK));
			outb(hayespbase + HAYESP_CMD2,
			     HAYESP_LOBYTE(HAYESP_RXHIWMARK));
			outb(hayespbase + HAYESP_CMD2,
			     HAYESP_HIBYTE(HAYESP_RXLOWMARK));
			outb(hayespbase + HAYESP_CMD2,
			     HAYESP_LOBYTE(HAYESP_RXLOWMARK));
		} else
#endif
		if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
			/* Set the FIFO threshold based on the receive speed. */
			outb(iobase + com_fifo,
			    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST |
			    (tp->t_ispeed <= 1200 ? FIFO_TRIGGER_1 : FIFO_TRIGGER_8));
		/* flush any pending I/O */
		while (ISSET(inb(iobase + com_lsr), LSR_RXRDY))
			(void) inb(iobase + com_data);
		/* you turn me on, baby */
		sc->sc_mcr = MCR_DTR | MCR_RTS;
		if (!ISSET(sc->sc_hwflags, COM_HW_NOIEN))
			SET(sc->sc_mcr, MCR_IENABLE);
		outb(iobase + com_mcr, sc->sc_mcr);
		outb(iobase + com_ier,
		    IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC);

		sc->sc_msr = inb(iobase + com_msr);
		if (ISSET(sc->sc_swflags, COM_SW_SOFTCAR) ||
		    ISSET(sc->sc_msr, MSR_DCD) || ISSET(tp->t_cflag, MDMBUF))
			SET(tp->t_state, TS_CARR_ON);
		else
			CLR(tp->t_state, TS_CARR_ON);
	} else if (ISSET(tp->t_state, TS_XCLUDE) && p->p_ucred->cr_uid != 0)
		return EBUSY;
	else
		s = spltty();

	/* wait for carrier if necessary */
	if (!ISSET(flag, O_NONBLOCK))
		while (!ISSET(tp->t_cflag, CLOCAL) &&
		    !ISSET(tp->t_state, TS_CARR_ON)) {
			SET(tp->t_state, TS_WOPEN);
			error = ttysleep(tp, &tp->t_rawq, TTIPRI | PCATCH,
			    ttopen, 0);
			if (error) {
				/* XXX should turn off chip if we're the
				   only waiter */
				splx(s);
				return error;
			}
		}
	splx(s);

	return (*linesw[tp->t_line].l_open)(dev, tp);
}
 
int
comclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = comcd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;
	int s;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*linesw[tp->t_line].l_close)(tp, flag);
	s = spltty();
	CLR(sc->sc_lcr, LCR_SBREAK);
	outb(iobase + com_lcr, sc->sc_lcr);
	outb(iobase + com_ier, 0);
	if (ISSET(tp->t_cflag, HUPCL) &&
	    !ISSET(sc->sc_swflags, COM_SW_SOFTCAR)) {
		/* XXX perhaps only clear DTR */
		outb(iobase + com_mcr, 0);
	}
	CLR(tp->t_state, TS_BUSY | TS_FLUSH);
	if (--comsopen == 0)
		untimeout(compoll, NULL);
	splx(s);
	ttyclose(tp);
#ifdef notyet /* XXXX */
	if (unit != comconsole) {
		ttyfree(tp);
		sc->sc_tty = 0;
	}
#endif
	return 0;
}
 
int
comread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}
 
int
comwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
comtty(dev)
	dev_t dev;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}
 
static u_char
tiocm_xxx2mcr(data)
	int data;
{
	u_char m = 0;

	if (ISSET(data, TIOCM_DTR))
		SET(m, MCR_DTR);
	if (ISSET(data, TIOCM_RTS))
		SET(m, MCR_RTS);
	return m;
}

int
comioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = COMUNIT(dev);
	struct com_softc *sc = comcd.cd_devs[unit];
	struct tty *tp = sc->sc_tty;
	int iobase = sc->sc_iobase;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	switch (cmd) {
	case TIOCSBRK:
		SET(sc->sc_lcr, LCR_SBREAK);
		outb(iobase + com_lcr, sc->sc_lcr);
		break;
	case TIOCCBRK:
		CLR(sc->sc_lcr, LCR_SBREAK);
		outb(iobase + com_lcr, sc->sc_lcr);
		break;
	case TIOCSDTR:
		SET(sc->sc_mcr, sc->sc_dtr);
		outb(iobase + com_mcr, sc->sc_mcr);
		break;
	case TIOCCDTR:
		CLR(sc->sc_mcr, sc->sc_dtr);
		outb(iobase + com_mcr, sc->sc_mcr);
		break;
	case TIOCMSET:
		CLR(sc->sc_mcr, MCR_DTR | MCR_RTS);
	case TIOCMBIS:
		SET(sc->sc_mcr, tiocm_xxx2mcr(*(int *)data));
		outb(iobase + com_mcr, sc->sc_mcr);
		break;
	case TIOCMBIC:
		CLR(sc->sc_mcr, tiocm_xxx2mcr(*(int *)data));
		outb(iobase + com_mcr, sc->sc_mcr);
		break;
	case TIOCMGET: {
		u_char m;
		int bits = 0;

		m = sc->sc_mcr;
		if (ISSET(m, MCR_DTR))
			SET(bits, TIOCM_DTR);
		if (ISSET(m, MCR_RTS))
			SET(bits, TIOCM_RTS);
		m = sc->sc_msr;
		if (ISSET(m, MSR_DCD))
			SET(bits, TIOCM_CD);
		if (ISSET(m, MSR_CTS))
			SET(bits, TIOCM_CTS);
		if (ISSET(m, MSR_DSR))
			SET(bits, TIOCM_DSR);
		if (ISSET(m, MSR_RI | MSR_TERI))
			SET(bits, TIOCM_RI);
		if (inb(iobase + com_ier))
			SET(bits, TIOCM_LE);
		*(int *)data = bits;
		break;
	}
	case TIOCGFLAGS: {
		int driverbits, userbits = 0;

		driverbits = sc->sc_swflags;
		if (ISSET(driverbits, COM_SW_SOFTCAR))
			SET(userbits, TIOCFLAG_SOFTCAR);
		if (ISSET(driverbits, COM_SW_CLOCAL))
			SET(userbits, TIOCFLAG_CLOCAL);
		if (ISSET(driverbits, COM_SW_CRTSCTS))
			SET(userbits, TIOCFLAG_CRTSCTS);
		if (ISSET(driverbits, COM_SW_MDMBUF))
			SET(userbits, TIOCFLAG_MDMBUF);

		*(int *)data = userbits;
		break;
	}
	case TIOCSFLAGS: {
		int userbits, driverbits = 0;

		error = suser(p->p_ucred, &p->p_acflag); 
		if (error != 0)
			return(EPERM); 

		userbits = *(int *)data;
		if (ISSET(userbits, TIOCFLAG_SOFTCAR) ||
		    ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			SET(driverbits, COM_SW_SOFTCAR);
		if (ISSET(userbits, TIOCFLAG_CLOCAL))
			SET(driverbits, COM_SW_CLOCAL);
		if (ISSET(userbits, TIOCFLAG_CRTSCTS))
			SET(driverbits, COM_SW_CRTSCTS);
		if (ISSET(userbits, TIOCFLAG_MDMBUF))
			SET(driverbits, COM_SW_MDMBUF);

		sc->sc_swflags = driverbits;
		break;
	}
	default:
		return ENOTTY;
	}

	return 0;
}

int
comparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(tp->t_dev)];
	int iobase = sc->sc_iobase;
	int ospeed = comspeed(t->c_ospeed);
	u_char lcr;
	tcflag_t oldcflag;
	int s;

	/* check requested parameters */
	if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
		return EINVAL;

	lcr = sc->sc_lcr & LCR_SBREAK;

	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		SET(lcr, LCR_5BITS);
		break;
	case CS6:
		SET(lcr, LCR_6BITS);
		break;
	case CS7:
		SET(lcr, LCR_7BITS);
		break;
	case CS8:
		SET(lcr, LCR_8BITS);
		break;
	}
	if (ISSET(t->c_cflag, PARENB)) {
		SET(lcr, LCR_PENAB);
		if (!ISSET(t->c_cflag, PARODD))
			SET(lcr, LCR_PEVEN);
	}
	if (ISSET(t->c_cflag, CSTOPB))
		SET(lcr, LCR_STOPB);

	sc->sc_lcr = lcr;

	s = spltty();

	if (ospeed == 0) {
		CLR(sc->sc_mcr, MCR_DTR);
		outb(iobase + com_mcr, sc->sc_mcr);
	}

	/*
	 * Set the FIFO threshold based on the receive speed, if we are
	 * changing it.
	 */
	if (tp->t_ispeed != t->c_ispeed) {
		if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
			outb(iobase + com_fifo,
			    FIFO_ENABLE |
			    (t->c_ispeed <= 1200 ? FIFO_TRIGGER_1 : FIFO_TRIGGER_8));
	}

	if (ospeed != 0) {
		outb(iobase + com_lcr, lcr | LCR_DLAB);
		outb(iobase + com_dlbl, ospeed);
		outb(iobase + com_dlbh, ospeed >> 8);
		outb(iobase + com_lcr, lcr);
		SET(sc->sc_mcr, MCR_DTR);
		outb(iobase + com_mcr, sc->sc_mcr);
	} else
		outb(iobase + com_lcr, lcr);

	/* When not using CRTSCTS, RTS follows DTR. */
	if (!ISSET(t->c_cflag, CRTSCTS)) {
		if (ISSET(sc->sc_mcr, MCR_DTR)) {
			if (!ISSET(sc->sc_mcr, MCR_RTS)) {
				SET(sc->sc_mcr, MCR_RTS);
				outb(iobase + com_mcr, sc->sc_mcr);
			}
		} else {
			if (ISSET(sc->sc_mcr, MCR_RTS)) {
				CLR(sc->sc_mcr, MCR_RTS);
				outb(iobase + com_mcr, sc->sc_mcr);
			}
		}
		sc->sc_dtr = MCR_DTR | MCR_RTS;
	} else
		sc->sc_dtr = MCR_DTR;

	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	oldcflag = tp->t_cflag;
	tp->t_cflag = t->c_cflag;

	/*
	 * If DCD is off and MDMBUF is changed, ask the tty layer if we should
	 * stop the device.
	 */
	if (!ISSET(sc->sc_msr, MSR_DCD) &&
	    !ISSET(sc->sc_swflags, COM_SW_SOFTCAR) &&
	    ISSET(oldcflag, MDMBUF) != ISSET(tp->t_cflag, MDMBUF) &&
	    (*linesw[tp->t_line].l_modem)(tp, 0) == 0) {
		CLR(sc->sc_mcr, sc->sc_dtr);
		outb(iobase + com_mcr, sc->sc_mcr);
	}

	splx(s);
	return 0;
}

void
comstart(tp)
	struct tty *tp;
{
	struct com_softc *sc = comcd.cd_devs[COMUNIT(tp->t_dev)];
	int iobase = sc->sc_iobase;
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_TTSTOP | TS_BUSY))
		goto out;
	if (ISSET(tp->t_cflag, CRTSCTS) && !ISSET(sc->sc_msr, MSR_CTS))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		if (tp->t_outq.c_cc == 0)
			goto out;
		selwakeup(&tp->t_wsel);
	}
	SET(tp->t_state, TS_BUSY);

#ifdef COM_HAYESP
	if (ISSET(sc->sc_hwflags, COM_HW_HAYESP)) {
		u_char buffer[1024], *cp = buffer;
		int n = q_to_b(&tp->t_outq, cp, sizeof buffer);
		do
			outb(iobase + com_data, *cp++);
		while (--n);
	}
	else
#endif
	if (ISSET(sc->sc_hwflags, COM_HW_FIFO)) {
		u_char buffer[16], *cp = buffer;
		int n = q_to_b(&tp->t_outq, cp, sizeof buffer);
		do {
			outb(iobase + com_data, *cp++);
		} while (--n);
	} else
		outb(iobase + com_data, getc(&tp->t_outq));
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
void
comstop(tp, flag)
	struct tty *tp;
{
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	splx(s);
}

void
comdiag(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	int overflows, floods;
	int s;

	s = spltty();
	sc->sc_errors = 0;
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	splx(s);

	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf overflow%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

void
compoll(arg)
	void *arg;
{
	int unit;
	struct com_softc *sc;
	struct tty *tp;
	register u_char *ibufp;
	u_char *ibufend;
	register int c;
	int s;
	static int lsrmap[8] = {
		0,      TTY_PE,
		TTY_FE, TTY_PE|TTY_FE,
		TTY_FE, TTY_PE|TTY_FE,
		TTY_FE, TTY_PE|TTY_FE
	};

	s = spltty();
	if (comevents == 0) {
		splx(s);
		goto out;
	}
	comevents = 0;
	splx(s);

	for (unit = 0; unit < comcd.cd_ndevs; unit++) {
		sc = comcd.cd_devs[unit];
		if (sc == 0 || sc->sc_ibufp == sc->sc_ibuf)
			continue;

		tp = sc->sc_tty;

		s = spltty();

		ibufp = sc->sc_ibuf;
		ibufend = sc->sc_ibufp;

		if (ibufp == ibufend) {
			splx(s);
			continue;
		}

		sc->sc_ibufp = sc->sc_ibuf = (ibufp == sc->sc_ibufs[0]) ?
					     sc->sc_ibufs[1] : sc->sc_ibufs[0];
		sc->sc_ibufhigh = sc->sc_ibuf + COM_IHIGHWATER;
		sc->sc_ibufend = sc->sc_ibuf + COM_IBUFSIZE;

		if (tp == 0 || !ISSET(tp->t_state, TS_ISOPEN)) {
			splx(s);
			continue;
		}

		if (ISSET(tp->t_cflag, CRTSCTS) &&
		    !ISSET(sc->sc_mcr, MCR_RTS)) {
			/* XXX */
			SET(sc->sc_mcr, MCR_RTS);
			outb(sc->sc_iobase + com_mcr, sc->sc_mcr);
		}

		splx(s);

		while (ibufp < ibufend) {
			c = *ibufp++;
			if (*ibufp & LSR_OE) {
				sc->sc_overflows++;
				if (sc->sc_errors++ == 0)
					timeout(comdiag, sc, 60 * hz);
			}
			/* This is ugly, but fast. */
			c |= lsrmap[(*ibufp++ & (LSR_BI|LSR_FE|LSR_PE)) >> 2];
			(*linesw[tp->t_line].l_rint)(c, tp);
		}
	}

out:
	timeout(compoll, NULL, 1);
}

int
comintr(arg)
	void *arg;
{
	struct com_softc *sc = arg;
	int iobase = sc->sc_iobase;
	struct tty *tp;
	u_char lsr, data, msr, delta;

	if (ISSET(inb(iobase + com_iir), IIR_NOPEND))
		return (0);

	tp = sc->sc_tty;

	for (;;) {
		lsr = inb(iobase + com_lsr);

		if (ISSET(lsr, LSR_RCV_MASK)) {
			register u_char *p = sc->sc_ibufp;

			comevents = 1;
			do {
				data = ISSET(lsr, LSR_RXRDY) ?
				    inb(iobase + com_data) : 0;
				if (ISSET(lsr, LSR_BI)) {
#ifdef DDB
					if (sc->sc_dev.dv_unit == comconsole) {
						Debugger();
						goto next;
					}
#endif
					data = '\0';
				}
				if (p >= sc->sc_ibufend) {
					sc->sc_floods++;
					if (sc->sc_errors++ == 0)
						timeout(comdiag, sc, 60 * hz);
				} else {
					*p++ = data;
					*p++ = lsr;
					if (p == sc->sc_ibufhigh &&
					    ISSET(tp->t_cflag, CRTSCTS)) {
						/* XXX */
						CLR(sc->sc_mcr, MCR_RTS);
						outb(iobase + com_mcr,
						     sc->sc_mcr);
					}
				}
			next:
				lsr = inb(iobase + com_lsr);
			} while (ISSET(lsr, LSR_RCV_MASK));

			sc->sc_ibufp = p;
		}
#if 0
		else if (ISSET(lsr, LSR_BI|LSR_FE|LSR_PE|LSR_OE))
			printf("weird lsr %02x\n", lsr);
#endif

		msr = inb(iobase + com_msr);

		if (msr != sc->sc_msr) {
			delta = msr ^ sc->sc_msr;
			sc->sc_msr = msr;
			if (ISSET(delta, MSR_DCD) &&
			    !ISSET(sc->sc_swflags, COM_SW_SOFTCAR) &&
			    (*linesw[tp->t_line].l_modem)(tp, ISSET(msr, MSR_DCD)) == 0) {
				CLR(sc->sc_mcr, sc->sc_dtr);
				outb(iobase + com_mcr, sc->sc_mcr);
			}
			if (ISSET(delta & msr, MSR_CTS) &&
			    ISSET(tp->t_cflag, CRTSCTS)) {
				/* the line is up and we want to do rts/cts flow control */
				(*linesw[tp->t_line].l_start)(tp);
			}
		}

		if (ISSET(lsr, LSR_TXRDY) && ISSET(tp->t_state, TS_BUSY)) {
			CLR(tp->t_state, TS_BUSY);
			if (ISSET(tp->t_state, TS_FLUSH))
				CLR(tp->t_state, TS_FLUSH);
			else
				(*linesw[tp->t_line].l_start)(tp);
		}

		if (ISSET(inb(iobase + com_iir), IIR_NOPEND))
			return (1);
	}
}

/*
 * Following are all routines needed for COM to act as console
 */
#include <dev/cons.h>

void
comcnprobe(cp)
	struct consdev *cp;
{

	if (!comprobe1(CONADDR)) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* locate the major number */
	for (commajor = 0; commajor < nchrdev; commajor++)
		if (cdevsw[commajor].d_open == comopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(commajor, CONUNIT);
#ifdef	COMCONSOLE
	cp->cn_pri = CN_REMOTE;		/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
	struct consdev *cp;
{

	cominit(CONUNIT, comdefaultrate);
	comconsole = CONUNIT;
	comconsinit = 0;
}

cominit(unit, rate)
	int unit, rate;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat;

	outb(iobase + com_lcr, LCR_DLAB);
	rate = comspeed(comdefaultrate);
	outb(iobase + com_dlbl, rate);
	outb(iobase + com_dlbh, rate >> 8);
	outb(iobase + com_lcr, LCR_8BITS);
	outb(iobase + com_ier, IER_ERXRDY | IER_ETXRDY);
	outb(iobase + com_fifo, FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_4);
	stat = inb(iobase + com_iir);
	splx(s);
}

comcngetc(dev)
	dev_t dev;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat, c;

	while (!ISSET(stat = inb(iobase + com_lsr), LSR_RXRDY))
		;
	c = inb(iobase + com_data);
	stat = inb(iobase + com_iir);
	splx(s);
	return c;
}

/*
 * Console kernel output character routine.
 */
void
comcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s = splhigh();
	int iobase = CONADDR;
	u_char stat;
	register int timo;

#ifdef KGDB
	if (dev != kgdb_dev)
#endif
	if (comconsinit == 0) {
		(void) cominit(COMUNIT(dev), comdefaultrate);
		comconsinit = 1;
	}
	/* wait for any pending transmission to finish */
	timo = 50000;
	while (!ISSET(stat = inb(iobase + com_lsr), LSR_TXRDY) && --timo)
		;
	outb(iobase + com_data, c);
	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(stat = inb(iobase + com_lsr), LSR_TXRDY) && --timo)
		;
	/* clear any interrupts generated by this transmission */
	stat = inb(iobase + com_iir);
	splx(s);
}

void
comcnpollc(dev, on)
	dev_t dev;
	int on;
{

}

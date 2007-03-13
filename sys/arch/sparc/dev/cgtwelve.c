/*	$OpenBSD: src/sys/arch/sparc/dev/cgtwelve.c,v 1.16 2007/03/13 19:40:48 miod Exp $	*/

/*
 * Copyright (c) 2002, 2003 Miodrag Vallat.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * cgtwelve (GS) accelerated 24-bit framebuffer driver.
 *
 * Enough experiments and SMI's cg12reg.h made this possible.
 */

/*
 * The cgtwelve framebuffer is a 3-slot SBUS card, that will fit only in
 * SPARCstation 1, 1+, 2 and 5, or in an xbox SBUS extension.
 *
 * It is a 24-bit 3D accelerated framebuffer made by Matrox, featuring 4MB
 * (regular model) or 8MB (high-res model) of video memory, a complex
 * windowing engine, double buffering modes, three video planes (overlay,
 * 8 bit and 24 bit color), and a lot of colormap combinations.
 *
 * All of this is driven by a set of three Bt462 ramdacs (latched unless
 * explicitely programmed), and a couple of other Matrox-specific chips.
 *
 * XXX The high res card is untested.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/conf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <machine/fbvar.h>

#include <sparc/dev/cgtwelvereg.h>
#include <sparc/dev/sbusvar.h>

#include <dev/cons.h>	/* for prom console hook */

/* per-display variables */
struct cgtwelve_softc {
	struct	sunfb	sc_sunfb;	/* common base device */
	struct	rom_reg sc_phys;

	volatile struct cgtwelve_dpu *sc_dpu;
	volatile struct cgtwelve_apu *sc_apu;
	volatile struct cgtwelve_dac *sc_ramdac;	/* RAMDAC registers */
	volatile u_char *sc_overlay;	/* overlay or enable plane */
	volatile u_long *sc_inten;	/* true color plane */

	int	sc_highres;
};

int	cgtwelve_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	cgtwelve_mmap(void *, off_t, int);
void	cgtwelve_prom(void *);
static __inline__
void	cgtwelve_ramdac_wraddr(struct cgtwelve_softc *, u_int32_t);
void	cgtwelve_reset(struct cgtwelve_softc *, int);

struct wsdisplay_accessops cgtwelve_accessops = {
	cgtwelve_ioctl,
	cgtwelve_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* scrollback */
	NULL,	/* getchar */
	NULL,	/* burner */
	NULL	/* pollc */
};

int	cgtwelvematch(struct device *, void *, void *);
void	cgtwelveattach(struct device *, struct device *, void *);

struct cfattach cgtwelve_ca = {
	sizeof(struct cgtwelve_softc), cgtwelvematch, cgtwelveattach
};

struct cfdriver cgtwelve_cd = {
	NULL, "cgtwelve", DV_DULL
};


int
cgtwelvematch(struct device *parent, void *vcf, void *aux)
{
	struct cfdata *cf = vcf;
	struct confargs *ca = aux;
	struct romaux *ra = &ca->ca_ra;

	if (strcmp(cf->cf_driver->cd_name, ra->ra_name) != 0)
		return (0);

	return (1);
}

void
cgtwelveattach(struct device *parent, struct device *self, void *args)
{
	struct cgtwelve_softc *sc = (struct cgtwelve_softc *)self;
	struct confargs *ca = args;
	int node;
	int isconsole = 0;
	char *ps;

	node = ca->ca_ra.ra_node;

	printf(": %s", getpropstring(node, "model"));
	ps = getpropstring(node, "dev_id");
	if (*ps != '\0')
		printf(" (%s)", ps);
	printf("\n");

	isconsole = node == fbnode;

	sc->sc_phys = ca->ca_ra.ra_reg[0];

	/*
	 * Map registers
	 */
	sc->sc_dpu = (struct cgtwelve_dpu *)mapiodev(ca->ca_ra.ra_reg,
	    CG12_OFF_DPU, sizeof(struct cgtwelve_dpu));
	sc->sc_apu = (struct cgtwelve_apu *)mapiodev(ca->ca_ra.ra_reg,
	    CG12_OFF_APU, sizeof(struct cgtwelve_apu));
	sc->sc_ramdac = (struct cgtwelve_dac *)mapiodev(ca->ca_ra.ra_reg,
	    CG12_OFF_DAC, sizeof(struct cgtwelve_dac));

	/*
	 * The console is using the 1-bit overlay plane, while the prom
	 * will correctly report 32 bit depth.
	 */
	fb_setsize(&sc->sc_sunfb, 1, CG12_WIDTH, CG12_HEIGHT,
	    node, ca->ca_bustype);
	sc->sc_sunfb.sf_depth = 1;
	sc->sc_sunfb.sf_linebytes = sc->sc_sunfb.sf_width / 8;
	sc->sc_sunfb.sf_fbsize = sc->sc_sunfb.sf_height *
	    sc->sc_sunfb.sf_linebytes;

	sc->sc_highres = sc->sc_sunfb.sf_width == CG12_WIDTH_HR;

	/*
	 * Map planes
	 */
	sc->sc_overlay = mapiodev(ca->ca_ra.ra_reg,
	    sc->sc_highres ? CG12_OFF_OVERLAY0_HR : CG12_OFF_OVERLAY0,
	    round_page(sc->sc_highres ? CG12_SIZE_OVERLAY_HR :
	        CG12_SIZE_OVERLAY));
	sc->sc_inten = mapiodev(ca->ca_ra.ra_reg,
	    sc->sc_highres ? CG12_OFF_INTEN_HR : CG12_OFF_INTEN,
	    round_page(sc->sc_highres ? CG12_SIZE_COLOR24_HR :
	        CG12_SIZE_COLOR24));

	/* reset cursor & frame buffer controls */
	sc->sc_sunfb.sf_depth = 0;	/* force action */
	cgtwelve_reset(sc, 1);

	sc->sc_sunfb.sf_ro.ri_bits = (void *)sc->sc_overlay;
	sc->sc_sunfb.sf_ro.ri_hw = sc;
	fbwscons_init(&sc->sc_sunfb, isconsole ? 0 : RI_CLEAR);

	if (isconsole) {
		fbwscons_console_init(&sc->sc_sunfb, -1);
		shutdownhook_establish(cgtwelve_prom, sc);
	}

	printf("%s: %dx%d", self->dv_xname,
	    sc->sc_sunfb.sf_width, sc->sc_sunfb.sf_height);
	ps = getpropstring(node, "ucoderev");
	if (*ps != '\0')
		printf(", microcode rev. %s", ps);
	printf("\n");

	fbwscons_attach(&sc->sc_sunfb, &cgtwelve_accessops, isconsole);
}

int
cgtwelve_ioctl(void *dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
	struct cgtwelve_softc *sc = dev;
	struct wsdisplay_fbinfo *wdf;

	/*
	 * Note that, although the emulation (text) mode is running in the
	 * overlay plane, we advertize the frame buffer as the full-blown
	 * 32-bit beast it is.
	 */
	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SUNCG12;
		break;
	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = sc->sc_sunfb.sf_height;
		wdf->width = sc->sc_sunfb.sf_width;
		wdf->depth = 32;
		wdf->cmsize = 0;
		break;
	case WSDISPLAYIO_GETSUPPORTEDDEPTH:
		*(u_int *)data = WSDISPLAYIO_DEPTH_24_32;
		break;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_sunfb.sf_linebytes * 32;
		break;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		break;

	case WSDISPLAYIO_SMODE:
		if (*(int *)data == WSDISPLAYIO_MODE_EMUL) {
			/* Back from X11 to text mode */
			cgtwelve_reset(sc, 1);
		} else {
			/* Starting X11, switch to 32 bit mode */
			cgtwelve_reset(sc, 32);
		}
		break;

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
		break;

	default:
		return (-1);	/* not supported yet */
	}

	return (0);
}

void
cgtwelve_reset(struct cgtwelve_softc *sc, int depth)
{
	u_int32_t c;

	if (sc->sc_sunfb.sf_depth != depth) {
		if (depth == 1) {
			/*
			 * Select the enable plane as sc_overlay, and fill it.
			 */
			sc->sc_apu->hpage = sc->sc_highres ?
			    CG12_HPAGE_ENABLE_HR : CG12_HPAGE_ENABLE;
			sc->sc_apu->haccess = CG12_HACCESS_ENABLE;
			sc->sc_dpu->pln_sl_host = CG12_PLN_SL_ENABLE;
			sc->sc_dpu->pln_rd_msk_host = CG12_PLN_RD_ENABLE;
			sc->sc_dpu->pln_wr_msk_host = CG12_PLN_WR_ENABLE;

			memset((void *)sc->sc_overlay, 0xff, sc->sc_highres ?
			    CG12_SIZE_ENABLE_HR : CG12_SIZE_ENABLE);

			/*
			 * Select the overlay plane as sc_overlay.
			 */
			sc->sc_apu->hpage = sc->sc_highres ?
			    CG12_HPAGE_OVERLAY_HR : CG12_HPAGE_OVERLAY;
			sc->sc_apu->haccess = CG12_HACCESS_OVERLAY;
			sc->sc_dpu->pln_sl_host = CG12_PLN_SL_OVERLAY;
			sc->sc_dpu->pln_rd_msk_host = CG12_PLN_RD_OVERLAY;
			sc->sc_dpu->pln_wr_msk_host = CG12_PLN_WR_OVERLAY;

			/*
			 * Upload a strict mono colormap, or the text
			 * upon returning from 32 bit mode would appear
			 * as (slightly dark) white on white.
			 */
			cgtwelve_ramdac_wraddr(sc, 0);
			sc->sc_ramdac->color = 0x00000000;
			for (c = 1; c < 256; c++)
				sc->sc_ramdac->color = 0x00ffffff;
		} else {
			/*
			 * Select the overlay plane as sc_overlay.
			 */
			sc->sc_apu->hpage = sc->sc_highres ?
			    CG12_HPAGE_OVERLAY_HR : CG12_HPAGE_OVERLAY;
			sc->sc_apu->haccess = CG12_HACCESS_OVERLAY;
			sc->sc_dpu->pln_sl_host = CG12_PLN_SL_OVERLAY;
			sc->sc_dpu->pln_rd_msk_host = CG12_PLN_RD_OVERLAY;
			sc->sc_dpu->pln_wr_msk_host = CG12_PLN_WR_OVERLAY;

			/*
			 * Do not attempt to somewhat preserve screen
			 * contents - reading the overlay plane and writing
			 * to the color plane at the same time is not
			 * reliable, and allocating memory to save a copy
			 * of the overlay plane would be awful.
			 */
			bzero((void *)sc->sc_overlay, sc->sc_highres ?
			    CG12_SIZE_OVERLAY_HR : CG12_SIZE_OVERLAY);

			/*
			 * Select the enable plane as sc_overlay, and clear it.
			 */
			sc->sc_apu->hpage = sc->sc_highres ?
			    CG12_HPAGE_ENABLE_HR : CG12_HPAGE_ENABLE;
			sc->sc_apu->haccess = CG12_HACCESS_ENABLE;
			sc->sc_dpu->pln_sl_host = CG12_PLN_SL_ENABLE;
			sc->sc_dpu->pln_rd_msk_host = CG12_PLN_RD_ENABLE;
			sc->sc_dpu->pln_wr_msk_host = CG12_PLN_WR_ENABLE;

			bzero((void *)sc->sc_overlay, sc->sc_highres ?
			    CG12_SIZE_ENABLE_HR : CG12_SIZE_ENABLE);

			/*
			 * Select the intensity (color) plane, and clear it.
			 */
			sc->sc_apu->hpage = sc->sc_highres ?
			    CG12_HPAGE_24BIT_HR : CG12_HPAGE_24BIT;
			sc->sc_apu->haccess = CG12_HACCESS_24BIT;
			sc->sc_dpu->pln_sl_host = CG12_PLN_SL_24BIT;
			sc->sc_dpu->pln_rd_msk_host = CG12_PLN_RD_24BIT;
			sc->sc_dpu->pln_wr_msk_host = CG12_PLN_WR_24BIT;

			memset((void *)sc->sc_inten, 0x00ffffff,
			    sc->sc_highres ?
			      CG12_SIZE_COLOR24_HR : CG12_SIZE_COLOR24);

			/*
			 * Use a direct colormap (ramp)
			 */
			cgtwelve_ramdac_wraddr(sc, 0);
			for (c = 0; c < 256; c++)
				sc->sc_ramdac->color = c | (c << 8) | (c << 16);
		}
	}

	sc->sc_sunfb.sf_depth = depth;
}

paddr_t
cgtwelve_mmap(void *v, off_t offset, int prot)
{
	struct cgtwelve_softc *sc = v;

	if (offset & PGOFSET || offset < 0)
		return (-1);

	/*
	 * Note that mmap() will invoke this function only if we are NOT
	 * in emulation mode, so we can assume 32 bit mode safely here.
	 */
	if (offset < sc->sc_sunfb.sf_fbsize * 32) {
		return (REG2PHYS(&sc->sc_phys,
		    (sc->sc_highres ? CG12_OFF_INTEN_HR :
		    CG12_OFF_INTEN) + offset) | PMAP_NC);
	}

	return (-1);
}

/*
 * Simple Bt462 programming routines.
 */

static __inline__ void 
cgtwelve_ramdac_wraddr(struct cgtwelve_softc *sc, u_int32_t addr)
{
	sc->sc_ramdac->addr_lo = (addr & 0xff);
	sc->sc_ramdac->addr_hi = ((addr >> 8) & 0xff);
}

/*
 * Shutdown hook used to restore PROM-compatible video mode on shutdown,
 * so that the PROM prompt is visible again.
 */
void
cgtwelve_prom(v)
	void *v;
{
	struct cgtwelve_softc *sc = v;
	extern struct consdev consdev_prom;

	if (sc->sc_sunfb.sf_depth != 1) {
		cgtwelve_reset(sc, 1);

		/*
		 * Go back to prom output for the last few messages, so they
		 * will be displayed correctly.
		 */
		cn_tab = &consdev_prom;
	}
}

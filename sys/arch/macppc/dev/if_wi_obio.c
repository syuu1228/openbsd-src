/*	$OpenBSD: src/sys/arch/macppc/dev/if_wi_obio.c,v 1.3 2001/09/19 20:18:57 miod Exp $	*/

/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	From: if_wi.c,v 1.7 1999/07/04 14:40:22 wpaul Exp $
 */

/*
 * Lucent WaveLAN/IEEE 802.11 PCMCIA driver for OpenBSD.
 *
 * Originally written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */
 /* 
  * modified for apple obio by Dale Rahn
  */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timeout.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net/if_ieee80211.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/ic/if_wireg.h>
#include <dev/ic/if_wi_ieee.h>
#include <dev/ic/if_wivar.h>

int	wi_obio_match		__P((struct device *, void *, void *));
void	wi_obio_attach	__P((struct device *, struct device *, void *));
int	wi_obio_detach	__P((struct device *, int));
int	wi_obio_activate	__P((struct device *, enum devact));
void	wi_obio_attach	__P((struct device *, struct device *, void *));
int	wi_obio_enable(struct wi_softc *sc);
void	wi_obio_disable(struct wi_softc *sc);


int	wi_intr			__P((void *));
int	wi_attach		__P((struct wi_softc *));
void	wi_init			__P((void *));
void	wi_stop			__P((struct wi_softc *));

struct wi_obio_softc {
	struct wi_softc sc_wi;
	u_int keywest;
};

struct cfattach wi_obio_ca = {
	sizeof (struct wi_obio_softc), wi_obio_match, wi_obio_attach,
	wi_obio_detach, wi_obio_activate
};


int
wi_obio_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "radio") == 0) 
		return (1);
	return (0);
}

void
wi_obio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wi_obio_softc	*psc = (struct wi_obio_softc *)self;
	struct wi_softc		*sc = &psc->sc_wi;
	struct confargs		*ca = aux;

	printf(" irq %d:", ca->ca_intr[0]);

	sc->wi_btag = ca->ca_iot;
	ca->ca_reg[0] += ca->ca_baseaddr;
	if (bus_space_map(sc->wi_btag, ca->ca_reg[0], ca->ca_reg[1], 0, &sc->wi_bhandle)) {
		printf("can't map i/o space\n");
	}
	/* FSCKING hackery */
	psc->keywest = (u_int) mapiodev(0x80000000, 0x1d000);

	/* Establish the interrupt. */
	mac_intr_establish(parent, ca->ca_intr[0], IST_LEVEL, IPL_NET,
		wi_intr, psc, "wi_obio");

	/* Make sure interrupts are disabled. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, 0xffff);

	wi_obio_enable(sc);

	wi_attach(sc);
}

int
wi_obio_detach(dev, flags)
	struct device *dev;
	int flags;
{
	struct wi_obio_softc *psc = (struct wi_obio_softc *)dev;
	struct wi_softc *sc = &psc->sc_wi;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	/*
	obio_io_unmap(psc->sc_pf, psc->sc_io_window);
	obio_io_free(psc->sc_pf, &psc->sc_pcioh);
	*/

	wi_obio_disable(sc);
	ether_ifdetach(ifp);
	if_detach(ifp);

	return (0);
}

int
wi_obio_activate(dev, act)
	struct device *dev;
	enum devact act;
{
	struct wi_obio_softc *psc = (struct wi_obio_softc *)dev;
	struct wi_softc *sc = &psc->sc_wi;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	int s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		wi_obio_enable(sc);
		printf("%s:", WI_PRT_ARG(sc));
		printf("\n");
		wi_init(sc);
		break;

	case DVACT_DEACTIVATE:
		ifp->if_timer = 0;
		if (ifp->if_flags & IFF_RUNNING)
			wi_stop(sc);
		wi_obio_disable(sc);
		break;
	}
	splx(s);
	return (0);
}

/* THIS IS CRAP */

int
wi_obio_enable(sc)
	struct wi_softc *sc;
{
	struct wi_obio_softc *psc = (struct wi_obio_softc *)sc;
	const u_int keywest = psc->keywest;	/* XXX */
	const u_int fcr2 = keywest + 0x40;
	const u_int gpio = keywest + 0x6a;
	const u_int extint_gpio = keywest + 0x58;
	u_int x;

	x = in32rb(fcr2);
	x |= 0x4;
	out32rb(fcr2, x);

	/* Enable card slot. */
	out8(gpio + 0x0f, 5);
	delay(1000);
	out8(gpio + 0x0f, 4);
	delay(1000);

	x = in32rb(fcr2);
	x &= ~0x8000000;

	out32rb(fcr2, x);
	/* out8(gpio + 0x10, 4); */

	out8(extint_gpio + 0x0b, 0);
	out8(extint_gpio + 0x0a, 0x28);
	out8(extint_gpio + 0x0d, 0x28);
	out8(gpio + 0x0d, 0x28);
	out8(gpio + 0x0e, 0x28);
	out32rb(keywest + 0x1c000, 0);

	/* Initialize the card. */
	out32rb(keywest + 0x1a3e0, 0x41);
	x = in32rb(fcr2);
	x |= 0x8000000;
	out32rb(fcr2, x);

	return 0;
}

void
wi_obio_disable(sc)
	struct wi_softc *sc;
{
	struct wi_obio_softc *psc = (struct wi_obio_softc *)sc;
	const u_int keywest = psc->keywest;	/* XXX */
	const u_int fcr2 = keywest + 0x40;
	u_int x;

	x = in32rb(fcr2);
	x &= ~0x4;
	out32rb(fcr2, x);
	/* out8(gpio + 0x10, 0); */
}

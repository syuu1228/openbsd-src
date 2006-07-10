/*	$OpenBSD: src/sys/dev/pci/ehci_pci.c,v 1.9 2006/07/10 07:54:43 dlg Exp $ */
/*	$NetBSD: ehci_pci.c,v 1.15 2004/04/23 21:13:06 itojun Exp $	*/

/*
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#ifdef EHCI_DEBUG
#define DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define DPRINTF(x)
#endif

struct ehci_pci_softc {
	ehci_softc_t		sc;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void 			*sc_ih;		/* interrupt vectoring */
};

int	ehci_pci_match(struct device *, void *, void *);
void	ehci_pci_attach(struct device *, struct device *, void *);
int	ehci_pci_detach(struct device *, int);
void	ehci_pci_givecontroller(struct ehci_pci_softc *);
void	ehci_pci_takecontroller(struct ehci_pci_softc *);
void	ehci_pci_shutdown(void *);

struct cfattach ehci_pci_ca = {
	sizeof(struct ehci_pci_softc), ehci_pci_match, ehci_pci_attach,
	ehci_pci_detach, ehci_activate
};


int
ehci_pci_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_EHCI)
		return (1);
 
	return (0);
}

void
ehci_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ehci_pci_softc *sc = (struct ehci_pci_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	char const *intrstr;
	pci_intr_handle_t ih;
	const char *vendor;
	char *devname = sc->sc.sc_bus.bdev.dv_xname;
	usbd_status r;

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_CBMEM, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc.sc_size, 0)) {
		printf(": can't map memory space\n");
		return;
	}

	sc->sc_pc = pc;
	sc->sc_tag = tag;
	sc->sc.sc_bus.dmatag = pa->pa_dmat;

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);
	DPRINTF(("%s: offs=%d\n", devname, sc->sc.sc_offs));
	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf(": couldn't map interrupt\n");
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_USB, ehci_intr, sc, devname);
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		return;
	}
	printf(": %s\n", intrstr);

	switch(pci_conf_read(pc, tag, PCI_USBREV) & PCI_USBREV_MASK) {
	case PCI_USBREV_PRE_1_0:
	case PCI_USBREV_1_0:
	case PCI_USBREV_1_1:
		sc->sc.sc_bus.usbrev = USBREV_UNKNOWN;
		printf("%s: pre-2.0 USB rev\n", devname);
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		return;
	case PCI_USBREV_2_0:
		sc->sc.sc_bus.usbrev = USBREV_2_0;
		break;
	default:
		sc->sc.sc_bus.usbrev = USBREV_UNKNOWN;
		break;
	}

	/* Figure out vendor for root hub descriptor. */
	vendor = pci_findvendor(pa->pa_id);
	sc->sc.sc_id_vendor = PCI_VENDOR(pa->pa_id);
	if (vendor)
		strlcpy(sc->sc.sc_vendor, vendor, sizeof(sc->sc.sc_vendor));
	else
		snprintf(sc->sc.sc_vendor, sizeof(sc->sc.sc_vendor),
		    "vendor 0x%04x", PCI_VENDOR(pa->pa_id));
	
	/* Enable workaround for dropped interrupts as required */
	if (sc->sc.sc_id_vendor == PCI_VENDOR_VIATECH)
		sc->sc.sc_flags |= EHCIF_DROPPED_INTR_WORKAROUND;

	ehci_pci_takecontroller(sc);
	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		return;
	}

	sc->sc.sc_shutdownhook = shutdownhook_establish(ehci_pci_shutdown, sc);

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

int
ehci_pci_detach(struct device *self, int flags)
{
	struct ehci_pci_softc *sc = (struct ehci_pci_softc *)self;
	int rv;

	rv = ehci_detach(&sc->sc, flags);
	if (rv)
		return (rv);
	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}
	return (0);
}

void
ehci_pci_givecontroller(struct ehci_pci_softc *sc)
{
	u_int32_t cparams, eec, legsup;
	int eecp;

	cparams = EREAD4(&sc->sc, EHCI_HCCPARAMS);
	for (eecp = EHCI_HCC_EECP(cparams); eecp != 0;
	    eecp = EHCI_EECP_NEXT(eec)) {
		eec = pci_conf_read(sc->sc_pc, sc->sc_tag, eecp);
		if (EHCI_EECP_ID(eec) != EHCI_EC_LEGSUP)
			continue;
		legsup = eec;
		pci_conf_write(sc->sc_pc, sc->sc_tag, eecp,
		    legsup & ~EHCI_LEGSUP_OSOWNED);
	}
}

void
ehci_pci_takecontroller(struct ehci_pci_softc *sc)
{
	u_int32_t cparams, eec, legsup;
	int eecp, i;

	cparams = EREAD4(&sc->sc, EHCI_HCCPARAMS);
	/* Synchronise with the BIOS if it owns the controller. */
	for (eecp = EHCI_HCC_EECP(cparams); eecp != 0;
	    eecp = EHCI_EECP_NEXT(eec)) {
		eec = pci_conf_read(sc->sc_pc, sc->sc_tag, eecp);
		if (EHCI_EECP_ID(eec) != EHCI_EC_LEGSUP)
			continue;
		legsup = eec;
		pci_conf_write(sc->sc_pc, sc->sc_tag, eecp,
		    legsup | EHCI_LEGSUP_OSOWNED);
		if (legsup & EHCI_LEGSUP_BIOSOWNED) {
			DPRINTF(("%s: waiting for BIOS to give up control\n",
			    USBDEVNAME(sc->sc.sc_bus.bdev)));
			for (i = 0; i < 5000; i++) {
				legsup = pci_conf_read(sc->sc_pc, sc->sc_tag,
				    eecp);
				if ((legsup & EHCI_LEGSUP_BIOSOWNED) == 0)
					break;
				DELAY(1000);
			}
			if (legsup & EHCI_LEGSUP_BIOSOWNED)
				printf("%s: timed out waiting for BIOS\n",
				    USBDEVNAME(sc->sc.sc_bus.bdev));
		}
	}
}

void
ehci_pci_shutdown(void *v)
{
	struct ehci_pci_softc *sc = (struct ehci_pci_softc *)v;

	ehci_shutdown(&sc->sc);
	ehci_pci_givecontroller(sc);
}

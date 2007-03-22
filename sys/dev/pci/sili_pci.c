/*	$OpenBSD: src/sys/dev/pci/sili_pci.c,v 1.1 2007/03/22 02:48:42 dlg Exp $ */

/*
 * Copyright (c) 2007 David Gwynne <dlg@openbsd.org>
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
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/silireg.h>
#include <dev/ic/silivar.h>

int	sili_pci_match(struct device *, void *, void *);
void	sili_pci_attach(struct device *, struct device *, void *);
int	sili_pci_detach(struct device *, int);

struct sili_pci_softc {
	struct sili_softc	psc_sili;

	pci_chipset_tag_t	psc_pc;
	pcitag_t		psc_tag;

	void			*psc_ih;
};

struct cfattach sili_pci_ca = {
	sizeof(struct sili_pci_softc), sili_pci_match, sili_pci_attach,
	sili_pci_detach
};

static const struct pci_matchid sili_devices[] = {
	{ PCI_VENDOR_CMDTECH,	PCI_PRODUCT_CMDTECH_3124 }
};

int
sili_pci_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid((struct pci_attach_args *)aux, sili_devices,
	    sizeof(sili_devices) / sizeof(sili_devices[0])));
}

void
sili_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct sili_pci_softc		*psc = (void *)self;
	struct sili_softc		*sc = &psc->psc_sili;
	struct pci_attach_args		*pa = aux;
//	pcireg_t			memtype;
	pci_intr_handle_t		ih;
	const char			*intrstr;

	psc->psc_pc = pa->pa_pc;
	psc->psc_tag = pa->pa_tag;
	psc->psc_ih = NULL;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_ios = 0;

#if 0
	/* find the appropriate memory base */
	for (r = PCI_MAPREG_START; r < PCI_MAPREG_END; r += sizeof(memtype)) {
		memtype = pci_mapreg_type(psc->psc_pc, psc->psc_tag, r);
		if ((memtype & PCI_MAPREG_TYPE_MASK) == PCI_MAPREG_TYPE_MEM)
			break;
	}
	if (r >= PCI_MAPREG_END) {
		printf(": unable to locate system interface registers\n");
		return;
	}

	if (pci_mapreg_map(pa, r, memtype, 0, &sc->sc_iot, &sc->sc_ioh,
	    NULL, &sc->sc_ios, 0) != 0) {
		printf(": unable to map system interface registers\n");
		return;
	}
#endif

	/* hook up the interrupt */
	if (pci_intr_map(pa, &ih)) {
		printf(": unable to map interrupt\n");
		goto unmap;
	}
	intrstr = pci_intr_string(psc->psc_pc, ih);
	psc->psc_ih = pci_intr_establish(psc->psc_pc, ih, IPL_BIO,
	    sili_intr, sc, sc->sc_dev.dv_xname);
	if (psc->psc_ih == NULL) {
		printf(": unable to map interrupt%s%s\n",
		    intrstr == NULL ? "" : " at ",
		    intrstr == NULL ? "" : intrstr);
		goto unmap;
	}
	printf(": %s", intrstr);

	if (sili_attach(sc) != 0) {
		/* error printed by sili_attach */
		goto deintr;
	}

	return;

deintr:
	pci_intr_disestablish(psc->psc_pc, psc->psc_ih);
	psc->psc_ih = NULL;
unmap:
//	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

int
sili_pci_detach(struct device *self, int flags)
{
	struct sili_pci_softc		*psc = (struct sili_pci_softc *)self;
	struct sili_softc		*sc = &psc->psc_sili;
	int				rv;

	rv = sili_detach(sc, flags);
	if (rv != 0)
		return (rv);

	if (psc->psc_ih != NULL) {
		pci_intr_disestablish(psc->psc_pc, psc->psc_ih);
		psc->psc_ih = NULL;
	}
	if (sc->sc_ios != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
		sc->sc_ios = 0;
	}

	return (0);
}

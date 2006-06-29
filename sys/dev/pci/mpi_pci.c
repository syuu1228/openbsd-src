/*	$OpenBSD: src/sys/dev/pci/mpi_pci.c,v 1.10 2006/06/29 22:55:26 brad Exp $ */

/*
 * Copyright (c) 2005 David Gwynne <dlg@openbsd.org>
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
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/ic/mpireg.h>
#include <dev/ic/mpivar.h>

int	mpi_pci_match(struct device *, void *, void *);
void	mpi_pci_attach(struct device *, struct device *, void *);
int	mpi_pci_detach(struct device *, int);

struct mpi_pci_softc {
	struct mpi_softc	psc_mpi;

	pci_chipset_tag_t	psc_pc;
	pcitag_t		psc_tag;

	void			*psc_ih;
};

struct cfattach mpi_pci_ca = {
	sizeof(struct mpi_pci_softc), mpi_pci_match, mpi_pci_attach,
	mpi_pci_detach
};

#define PREAD(s, r)	pci_conf_read((s)->psc_pc, (s)->psc_tag, (r))
#define PWRITE(s, r, v)	pci_conf_write((s)->psc_pc, (s)->psc_tag, (r), (v))

static const struct pci_matchid mpi_devices[] = {
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_1030 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC909 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC909A },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC919 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC919_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC929 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_FC929_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1064 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1064A },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1064E },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1066 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1066E },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1068 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1078 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS1068E }
};

int
mpi_pci_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid((struct pci_attach_args *)aux, mpi_devices,
	    sizeof(mpi_devices) / sizeof(mpi_devices[0])));
}

void
mpi_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct mpi_pci_softc		*psc = (void *)self;
	struct mpi_softc		*sc = &psc->psc_mpi;
	struct pci_attach_args		*pa = aux;
	pcireg_t			memtype;
	int				r;
	pci_intr_handle_t		ih;
	const char			*intrstr;

	psc->psc_pc = pa->pa_pc;
	psc->psc_tag = pa->pa_tag;
	psc->psc_ih = NULL;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_ios = 0;

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

	/* disable the expansion rom */
	PWRITE(psc, PCI_ROM_REG, PREAD(psc, PCI_ROM_REG & ~PCI_ROM_ENABLE));

	/* hook up the interrupt */
	if (pci_intr_map(pa, &ih)) {
		printf(": unable to map interrupt\n");
		goto unmap;
	}
	intrstr = pci_intr_string(psc->psc_pc, ih);
	psc->psc_ih = pci_intr_establish(psc->psc_pc, ih, IPL_BIO,
	    mpi_intr, sc, sc->sc_dev.dv_xname);
	if (psc->psc_ih == NULL) {
		printf(": unable to map interrupt%s%s\n",
		    intrstr == NULL ? "" : " at ",
		    intrstr == NULL ? "" : intrstr);
		goto unmap;
	}
	printf(": %s", intrstr);

	/* vmware doesn't fill in the pci subsystem register */
	if (pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG) == 0x0)
		sc->sc_flags |= MPI_F_VMWARE;

	if (mpi_attach(sc) != 0) {
		/* error printed by mpi_attach */
		goto deintr;
	}

	return;

deintr:
	pci_intr_disestablish(psc->psc_pc, psc->psc_ih);
	psc->psc_ih = NULL;
unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

int
mpi_pci_detach(struct device *self, int flags)
{
	struct mpi_pci_softc		*psc = (struct mpi_pci_softc *)self;
	struct mpi_softc		*sc = &psc->psc_mpi;

	mpi_detach(sc);

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
